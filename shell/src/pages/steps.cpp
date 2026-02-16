#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>

#include "tex/tex.h"
#include "tex/tex_renderer.h"

#include <utility>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::keep_cursor_in_view;
using detail::kKbGroup1;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;

static TeX_Config kTeXCfg{ui::color::kBlack, ui::color::kWhite, "TeXFonts", nullptr, nullptr};
} // namespace

void App::steps_tex_reset() noexcept {
		steps_tex_release();
}

void App::steps_tex_release() noexcept {
		tex_cached_step_index_ = 0xFFFF;
		tex_cached_step_ec_ = matrix_core::ErrorCode::Internal;
		tex_scroll_y_ = 0;
		tex_total_height_ = 0;
		tex_doc_[0] = '\0';
		if (tex_layout_) {
				if (tex_renderer_)
						tex_renderer_invalidate(tex_renderer_);
				tex_free(tex_layout_);
				tex_layout_ = nullptr;
		}
}

#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
void App::render_cramer_steps_menu(const CramerStepsMenuState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header("Cramer Steps");
		render_footer_hint("ENTER: Select  CLEAR: Back");

		const std::uint8_t count = static_cast<std::uint8_t>(s.n + 1u);

		const int list_top = l.header_h + l.margin_y;
		const int line_h = 14;
		const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;

		for (std::uint8_t row = 0; row < visible; row++) {
				const std::uint8_t idx = static_cast<std::uint8_t>(s.scroll + row);
				if (idx >= count)
						break;

				const int y = list_top + row * line_h;
				const bool selected = (idx == s.cursor);
				if (selected) {
						gfx_SetColor(ui::color::kBlue);
						gfx_FillRectangle(0, y - 1, kScreenW, line_h);
						gfx_SetTextFGColor(ui::color::kWhite);
				} else {
						gfx_SetTextFGColor(ui::color::kBlack);
				}

				gfx_SetTextXY(l.margin_x, y + 2);
				if (idx == 0) {
						gfx_PrintString("Delta = det(A)");
				} else {
						gfx_PrintString("Delta");
						gfx_PrintChar('_');
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append_u64(static_cast<std::uint64_t>(idx));
						gfx_PrintString(fmt_buf_);
				}
		}
}
#else
void App::render_cramer_steps_menu(const CramerStepsMenuState&) noexcept {
		const ui::Layout l = ui::Layout{};
		render_header("Cramer Steps");
		render_footer_hint("CLEAR: Back");
		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 30);
		gfx_PrintString("Disabled in this build.");
}
#endif

void App::render_steps(const StepsState& s) noexcept {
		const ui::Layout l = ui::Layout{};
		render_header("Steps");

		if (!expl_.available()) {
				gfx_SetTextFGColor(ui::color::kBlack);
				gfx_SetTextXY(l.margin_x, l.header_h + 30);
				gfx_PrintString("No steps available.");
				return;
		}

		const std::size_t n = expl_.step_count();
		if (n == 0) {
				gfx_SetTextFGColor(ui::color::kBlack);
				gfx_SetTextXY(l.margin_x, l.header_h + 30);
				gfx_PrintString("No steps.");
				return;
		}

		if (static_cast<std::size_t>(s.index) >= n)
				fail_fast("render_steps: step index out of range");
		const std::size_t idx = static_cast<std::size_t>(s.index);

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 4);
		gfx_PrintString("Steps");
		gfx_PrintString(" ");
		fmt_buf_[0] = '\0';
		matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
		w.append_u64(static_cast<std::uint64_t>(idx + 1));
		w.put('/');
		w.append_u64(static_cast<std::uint64_t>(n));
		gfx_PrintString(fmt_buf_);

		const int content_y = l.header_h + 18;

		// build and cache core step buffers + tex layout only when the step changes
		if (tex_cached_step_index_ != static_cast<std::uint16_t>(idx)) {
				steps_tex_release();

				step_caption_[0] = '\0';
				step_latex_[0] = '\0';

				{
						matrix_core::ArenaScratchScope scratch_tx(scratch_);
						matrix_core::StepRenderBuffers out;
						out.caption = step_caption_;
						out.caption_cap = sizeof(step_caption_);
						out.latex = step_latex_;
						out.latex_cap = sizeof(step_latex_);
						out.scratch = &scratch_;

						tex_cached_step_ec_ = expl_.render_step(idx, out);
				}
				SHELL_DBG("[steps] render idx=%u/%u ec=%u\n", (unsigned)idx, (unsigned)n, (unsigned)tex_cached_step_ec_);

				if (!matrix_core::is_ok(tex_cached_step_ec_)) {
						gfx_SetTextFGColor(ui::color::kBlack);
						gfx_SetTextXY(l.margin_x, content_y);
						gfx_PrintString("Step render failed.");
						render_footer_hint("LEFT/RIGHT: Step  2ND+LEFT/RIGHT: Ends  CLEAR: Back");
						return;
				}

				tex_doc_[0] = '\0';
				matrix_core::CheckedWriter texw{tex_doc_, sizeof(tex_doc_)};
				if (step_caption_[0] != '\0') {
						texw.append(step_caption_);
						texw.put('\n');
				}
				texw.append(step_latex_);

				tex_layout_ = tex_format(tex_doc_, kScreenW - 2 * l.margin_x, &kTeXCfg);
				if (!tex_layout_) {
						SHELL_DBG("[tex] tex_format failed\n");
						gfx_SetTextXY(l.margin_x, content_y);
						gfx_PrintString("TeX format failed.");
						render_footer_hint("LEFT/RIGHT: Step  2ND+LEFT/RIGHT: Ends  CLEAR: Back");
						return;
				}
				tex_total_height_ = tex_get_total_height(tex_layout_);
				tex_cached_step_index_ = static_cast<std::uint16_t>(idx);
				tex_scroll_y_ = 0;
		}

		if (!matrix_core::is_ok(tex_cached_step_ec_)) {
				gfx_SetTextFGColor(ui::color::kBlack);
				gfx_SetTextXY(l.margin_x, content_y);
				gfx_PrintString("Step render failed.");
				render_footer_hint("LEFT/RIGHT: Step  2ND+LEFT/RIGHT: Ends  CLEAR: Back");
				return;
		}

		if (tex_layout_ && tex_renderer_) {
				// libtexce uses a fixed 240px viewport. render first, then redraw footer to
				// keep UI on top
				const int view_h = kScreenH - content_y - l.footer_h;
				int max_scroll = tex_total_height_ - view_h;
				if (max_scroll < 0)
						max_scroll = 0;
				if (tex_scroll_y_ < 0)
						tex_scroll_y_ = 0;
				if (tex_scroll_y_ > max_scroll)
						tex_scroll_y_ = max_scroll;

				tex_draw(tex_renderer_, tex_layout_, l.margin_x, content_y, tex_scroll_y_);
		}

		render_footer_hint("LEFT/RIGHT: Step  2ND+LEFT/RIGHT: Ends  UP/DOWN: Scroll  CLEAR: Back");
}

#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
void App::update_cramer_steps_menu(CramerStepsMenuState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[cramer] CLEAR back -> drop expl\n");
				REQUIRE(pop(), "pop failed");
				persist_.rewind(persist_tail_mark_);
				expl_ = {};
				return;
		}

		const std::uint8_t count = static_cast<std::uint8_t>(s.n + 1u);

		const std::uint8_t old_cursor = s.cursor;
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.cursor > 0)
						s.cursor--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.cursor + 1 < count)
						s.cursor++;
		}
		if (s.cursor != old_cursor) {
				const int line_h = 14;
				const ui::Layout l = ui::Layout{};
				const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;
				s.scroll = keep_cursor_in_view(s.cursor, s.scroll, count, visible);
		}

		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		if (s.slot_a >= kSlotCount || s.slot_b >= kSlotCount)
				fail_fast("update_cramer_steps_menu: slot out of range");
		if (!slots_[s.slot_a].is_set() || !slots_[s.slot_b].is_set())
				fail_fast("update_cramer_steps_menu: expected slots to be set");

		const matrix_core::MatrixView a = slots_[s.slot_a].view_active();
		const matrix_core::MatrixView b = slots_[s.slot_b].view_active();

		// replace any previous Δ/Δ_i explanation but keep the result matrix
		persist_.rewind(persist_tail_mark_);
		expl_ = {};

		matrix_core::ArenaScope tx(persist_);
		matrix_core::ArenaScratchScope scratch_tx(scratch_);
		matrix_core::ExplainOptions opts_steps;
		opts_steps.enable = true;
		opts_steps.persist = &persist_;

		matrix_core::Explanation expl;
		matrix_core::Rational v = matrix_core::Rational::from_int(0);
		matrix_core::Error err{};

		if (s.cursor == 0) {
				SHELL_DBG("[cramer] run det(A)\n");
				err = matrix_core::op_det(a, scratch_, &v, &expl, opts_steps);
		} else {
				const std::uint8_t col = static_cast<std::uint8_t>(s.cursor - 1u);
				SHELL_DBG("[cramer] run det_replace col=%u\n", (unsigned)col);
				err = matrix_core::op_det_replace_column(a, b, col, scratch_, &v, &expl, opts_steps);
		}

		SHELL_DBG("[cramer] step err=%u\n", (unsigned)err.code);
		if (!matrix_core::is_ok(err)) {
				if (err.code == matrix_core::ErrorCode::NotSquare || err.code == matrix_core::ErrorCode::DimensionMismatch ||
				        err.code == matrix_core::ErrorCode::IndexOutOfRange || err.code == matrix_core::ErrorCode::InvalidDimension ||
				        err.code == matrix_core::ErrorCode::Internal) {
						fail_fast("update_cramer_steps_menu: unexpected error");
				}
				show_message("Error");
				return;
		}

		tx.commit();
		expl_ = std::move(expl);
		REQUIRE(push(Page::make_steps()), "push steps failed");
}
#else
void App::update_cramer_steps_menu(CramerStepsMenuState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				REQUIRE(pop(), "pop failed");
				return;
		}
		(void)s;
}
#endif

void App::update_steps(StepsState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[steps] CLEAR back\n");
				steps_tex_release();
				REQUIRE(pop(), "pop failed");
				return;
		}

		if (!expl_.available())
				return;

		const std::size_t n = expl_.step_count();
		if (n == 0)
				return;

		const bool jump = input_.down(kKbGroup1, kb_2nd);

		if (jump && input_.repeat(kKbGroupArrows, kb_Left)) {
				s.index = 0;
				steps_tex_reset();
				return;
		}
		if (jump && input_.repeat(kKbGroupArrows, kb_Right)) {
				s.index = static_cast<std::uint16_t>(n - 1);
				steps_tex_reset();
				return;
		}

		if (input_.repeat(kKbGroupArrows, kb_Left)) {
				if (s.index > 0)
						s.index--;
				steps_tex_reset();
				return;
		}
		if (input_.repeat(kKbGroupArrows, kb_Right)) {
				if (static_cast<std::size_t>(s.index + 1) < n)
						s.index++;
				steps_tex_reset();
				return;
		}

		// vertical scroll (not sure when this would ever be the case, but just in case)
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				tex_scroll_y_ -= 10;
				return;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				tex_scroll_y_ += 10;
				return;
		}
}

} // namespace matrix_shell
