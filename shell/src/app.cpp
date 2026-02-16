#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"
#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/writer.hpp"

#include <fontlibc.h>
#include <graphx.h>
#include <keypadc.h>

#include "tex/tex.h"
#include "tex/tex_renderer.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace matrix_shell {
namespace {
using detail::dbg_print_i64;
using detail::dbg_print_matrix;
using detail::dbg_print_rational;
using detail::fail_fast;
using detail::keep_cursor_in_view;
using detail::kKbGroup1;
using detail::kKbGroup3;
using detail::kKbGroup4;
using detail::kKbGroup5;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
using detail::op_is_binary;
using detail::op_name;

constexpr std::size_t kSlabBytes = 18u * 1024u;
constexpr std::size_t kPersistBytes = 9u * 1024u;
constexpr std::size_t kScratchBytes = 9u * 1024u;

static_assert(kPersistBytes + kScratchBytes == kSlabBytes);

constexpr std::size_t kTeXRendererBytes = 20u * 1024u;

void print_single_line_clipped(const char* text, int max_width_px) noexcept {
		if (!text || max_width_px <= 0)
				return;
		int used_px = 0;
		for (const char* p = text; *p != '\0'; ++p) {
				const char c = *p;
				if (c == '\n' || c == '\r')
						break;
				const int char_w = static_cast<int>(gfx_GetCharWidth(c));
				if (used_px + char_w > max_width_px)
						break;
				gfx_PrintChar(c);
				used_px += char_w;
		}
}

} // namespace

bool App::init() noexcept {
		const matrix_core::ErrorCode ec = slab_.init(kSlabBytes);
		if (!matrix_core::is_ok(ec))
				return false;

		persist_.reset(slab_.data(), kPersistBytes);
		scratch_.reset(slab_.data() + kPersistBytes, kScratchBytes);
		persist_base_mark_ = persist_.mark();
		persist_tail_mark_ = persist_base_mark_;

		depth_ = 0;
		push_root();
		msg_ = {};

		SHELL_DBG("[init] slab=%uB persist=%uB scratch=%uB\n", (unsigned)kSlabBytes, (unsigned)kPersistBytes, (unsigned)kScratchBytes);
		SHELL_DBG("[init] persist base=%p cap=%u\n", (void*)slab_.data(), (unsigned)persist_.capacity());
		SHELL_DBG("[init] scratch base=%p cap=%u\n", (void*)(slab_.data() + kPersistBytes), (unsigned)scratch_.capacity());
		SHELL_DBG("[init] persist_base_mark=%u\n", (unsigned)persist_base_mark_);

		fontlib_font_t* f_main = fontlib_GetFontByIndex("TeXFonts", 0);
		fontlib_font_t* f_script = fontlib_GetFontByIndex("TeXScrpt", 0);
		if (!f_main || !f_script) {
				SHELL_DBG("[tex] missing TeXFonts/TeXScrpt appvars\n");
				return false;
		}
		tex_draw_set_fonts(f_main, f_script);
		fontlib_SetForegroundColor(ui::color::kBlack);
		fontlib_SetBackgroundColor(ui::color::kWhite);
		fontlib_SetTransparency(true);

		tex_renderer_ = tex_renderer_create_sized(kTeXRendererBytes);
		if (!tex_renderer_) {
				SHELL_DBG("[tex] tex_renderer_create_sized(%u) failed\n", (unsigned)kTeXRendererBytes);
				return false;
		}
		{
				size_t peak = 0, cap = 0, alloc = 0, reset = 0;
				tex_renderer_get_stats(tex_renderer_, &peak, &cap, &alloc, &reset);
				SHELL_DBG("[tex] renderer cap=%uB\n", (unsigned)cap);
		}

		return true;
}

void App::push_root() noexcept {
		depth_ = 0;
		stack_[0] = Page::make_menu(MenuId::Main);
		depth_ = 1;
		SHELL_DBG("[nav] push_root -> Main\n");
}

Page& App::top() noexcept {
		if (depth_ == 0)
				fail_fast("top(): empty page stack");
		return stack_[depth_ - 1];
}

const Page& App::top() const noexcept {
		if (depth_ == 0)
				fail_fast("top() const: empty page stack");
		return stack_[depth_ - 1];
}

bool App::push(const Page& p) noexcept {
		if (depth_ >= kMaxPageDepth)
				return false;
		stack_[depth_++] = p;
		SHELL_DBG("[nav] push kind=%u depth=%u\n", (unsigned)p.kind, (unsigned)depth_);
		if (p.kind == PageKind::Steps)
				steps_tex_reset();
		return true;
}

bool App::pop() noexcept {
		if (depth_ <= 1)
				return false;
		depth_--;
		SHELL_DBG("[nav] pop depth=%u\n", (unsigned)depth_);
		return true;
}

void App::show_message(const char* msg) noexcept {
		msg_.active = true;
		msg_.frames_left = 90;
		msg_.text[0] = '\0';
		matrix_core::CheckedWriter w{msg_.text, sizeof(msg_.text)};
		w.append(msg ? msg : "");
		SHELL_DBG("[msg] %s\n", msg_.text);
}

void App::update_message() noexcept {
		if (!msg_.active)
				return;
		if (msg_.frames_left > 0)
				msg_.frames_left--;
		if (msg_.frames_left == 0)
				msg_.active = false;
}

matrix_core::ErrorCode App::ensure_slot_allocated(std::uint8_t slot) noexcept {
		if (slot >= kSlotCount)
				fail_fast("ensure_slot_allocated: slot out of range");
		if (slots_[slot].allocated())
				return matrix_core::ErrorCode::Ok;

		persist_.rewind(persist_base_mark_);
		persist_tail_mark_ = persist_base_mark_;
		expl_ = {};
		matrix_core::ArenaScope tx(persist_);

		matrix_core::MatrixMutView backing;
		const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, 6, 6, &backing);
		if (!matrix_core::is_ok(ec)) {
				SHELL_DBG("[slot] alloc %c failed ec=%u used=%u/%u\n",
				        (char)('A' + slot),
				        (unsigned)ec,
				        (unsigned)persist_.used(),
				        (unsigned)persist_.capacity());
				return ec;
		}

		slots_[slot].backing = backing;
		slots_[slot].rows = 2;
		slots_[slot].cols = 2;

		persist_base_mark_ = persist_.mark();
		persist_tail_mark_ = persist_base_mark_;
		tx.commit();
		SHELL_DBG("[slot] alloc %c backing=%p base_mark=%u used=%u/%u\n",
		        (char)('A' + slot),
		        (void*)backing.data,
		        (unsigned)persist_base_mark_,
		        (unsigned)persist_.used(),
		        (unsigned)persist_.capacity());
		return matrix_core::ErrorCode::Ok;
}

void App::clear_slot(std::uint8_t slot) noexcept {
		if (slot >= kSlotCount)
				fail_fast("clear_slot: slot out of range");
		if (!slots_[slot].allocated())
				fail_fast("clear_slot: slot not allocated");

		matrix_core::matrix_fill_zero(slots_[slot].backing);
		slots_[slot].rows = 0;
		slots_[slot].cols = 0;
		SHELL_DBG("[slot] cleared %c\n", (char)('A' + slot));
}

bool App::parse_i64(const char* s, std::int64_t* out) noexcept {
		if (!out || !s || s[0] == '\0')
				return false;

		const char* const end = s + std::strlen(s);
		char* parse_end = nullptr;
		errno = 0;
		const long long v = std::strtoll(s, &parse_end, 10);
		if (errno != 0)
				return false;
		if (parse_end != end)
				return false;
		*out = static_cast<std::int64_t>(v);
		return true;
}

bool App::step() noexcept {
		input_.begin_frame();
		update_message();

		// global back/exit handling
		const bool clear_pressed = input_.pressed(kKbGroup6, kb_Clear);
		if (clear_pressed && depth_ == 1) {
				SHELL_DBG("[nav] exit via CLEAR at root\n");
				steps_tex_release();
				if (tex_renderer_) {
						tex_renderer_destroy(tex_renderer_);
						tex_renderer_ = nullptr;
				}
				return false;
		}

		Page& p = top();
		switch (p.kind) {
		case PageKind::Menu:
				update_menu(p.u.menu);
				break;
		case PageKind::Dim:
				update_dim(p.u.dim);
				break;
		case PageKind::Editor:
				update_editor(p.u.editor);
				break;
		case PageKind::SlotPick:
				update_slot_pick(p.u.pick);
				break;
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
		case PageKind::CofactorElement:
				update_cofactor_element(p.u.cofactor);
				break;
#endif
		case PageKind::Result:
				update_result(p.u.result);
				break;
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
		case PageKind::ProjectionResult:
				update_projection_result(p.u.proj);
				break;
#endif
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
		case PageKind::CramerStepsMenu:
				update_cramer_steps_menu(p.u.cramer_steps);
				break;
#endif
		case PageKind::Steps:
				update_steps(p.u.steps);
				break;
		case PageKind::Confirm:
				update_confirm(p.u.confirm);
				break;
		default:
				fail_fast("Unhandled PageKind in step()");
		}

		render();
		gfx_SwapDraw();
		return true;
}

void App::render_header(const char* title) noexcept {
		const ui::Layout l = ui::Layout{};
		gfx_SetColor(ui::color::kLightGray);
		gfx_FillRectangle(0, 0, kScreenW, l.header_h);

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, 4);
		if (title)
				gfx_PrintString(title);
}

void App::render_footer_hint(const char* hint) noexcept {
		const ui::Layout l = ui::Layout{};
		const int y = kScreenH - l.footer_h;
		gfx_SetColor(ui::color::kLightGray);
		gfx_FillRectangle(0, y, kScreenW, l.footer_h);

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, y + 6);
		print_single_line_clipped(hint, kScreenW - 2 * l.margin_x);
}

void App::render_message() noexcept {
		if (!msg_.active)
				return;
		const ui::Layout l = ui::Layout{};

		const int y = kScreenH - l.footer_h - 18;
		gfx_SetColor(ui::color::kDarkGray);
		gfx_FillRectangle(l.margin_x, y, kScreenW - 2 * l.margin_x, 16);

		gfx_SetTextFGColor(ui::color::kWhite);
		gfx_SetTextXY(l.margin_x + 4, y + 4);
		gfx_PrintString(msg_.text);
}

void App::render() noexcept {
		gfx_SetColor(ui::color::kWhite);
		gfx_FillScreen(ui::color::kWhite);

		Page& p = top();
		switch (p.kind) {
		case PageKind::Menu:
				render_menu(p.u.menu);
				break;
		case PageKind::Dim:
				render_dim(p.u.dim);
				break;
		case PageKind::Editor:
				render_editor(p.u.editor);
				break;
		case PageKind::SlotPick:
				render_slot_pick(p.u.pick);
				break;
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
		case PageKind::CofactorElement:
				render_cofactor_element(p.u.cofactor);
				break;
#endif
		case PageKind::Result:
				render_result(p.u.result);
				break;
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
		case PageKind::ProjectionResult:
				render_projection_result(p.u.proj);
				break;
#endif
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
		case PageKind::CramerStepsMenu:
				render_cramer_steps_menu(p.u.cramer_steps);
				break;
#endif
		case PageKind::Steps:
				render_steps(p.u.steps);
				break;
		case PageKind::Confirm:
				render_confirm(p.u.confirm);
				break;
		default:
				fail_fast("Unhandled PageKind in render()");
		}

		render_message();
}

} // namespace matrix_shell
