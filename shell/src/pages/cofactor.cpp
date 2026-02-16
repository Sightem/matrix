#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>

#include <utility>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenW;
} // namespace

#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
void App::render_cofactor_element(const CofactorElementState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header("Cofactor (Element)");
		render_footer_hint("UP/DOWN: Field  LEFT/RIGHT: Adjust  ENTER: Run  CLEAR: Back");

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 10);
		gfx_PrintString("Matrix ");
		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintString(" (n=");
		gfx_PrintChar(static_cast<char>('0' + s.n));
		gfx_PrintString(")");

		const int y_base = l.header_h + 32;
		const int line_h = 16;

		auto draw_field = [&](int y, const char* name, const char* value, bool selected) noexcept {
				if (selected) {
						gfx_SetColor(ui::color::kBlue);
						gfx_FillRectangle(0, y - 1, kScreenW, 14);
						gfx_SetTextFGColor(ui::color::kWhite);
				} else {
						gfx_SetTextFGColor(ui::color::kBlack);
				}
				gfx_SetTextXY(l.margin_x, y + 2);
				gfx_PrintString(name);
				gfx_PrintString(": ");
				gfx_PrintString(value);
		};

		fmt_buf_[0] = '\0';
		{
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append_u64(static_cast<std::uint64_t>(s.i) + 1u);
		}
		draw_field(y_base + 0 * line_h, "Row i", fmt_buf_, s.focus == 0);

		fmt_buf_[0] = '\0';
		{
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append_u64(static_cast<std::uint64_t>(s.j) + 1u);
		}
		draw_field(y_base + 1 * line_h, "Col j", fmt_buf_, s.focus == 1);
}
#else
void App::render_cofactor_element(const CofactorElementState& s) noexcept {
		const ui::Layout l = ui::Layout{};
		render_header("Cofactor (Element)");
		render_footer_hint("CLEAR: Back");
		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 30);
		gfx_PrintString("Disabled in this build.");
		(void)s;
}
#endif

#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
void App::update_cofactor_element(CofactorElementState& s) noexcept {
		// back
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[cof] CLEAR back\n");
				REQUIRE(pop(), "pop failed");
				return;
		}

		// field nav
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.focus > 0)
						s.focus--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.focus < 1)
						s.focus++;
		}

		// adjust focused field
		if (s.focus == 0) {
				if (input_.repeat(kKbGroupArrows, kb_Left) && s.i > 0)
						s.i--;
				if (input_.repeat(kKbGroupArrows, kb_Right) && (s.i + 1u) < s.n)
						s.i++;
		} else {
				if (input_.repeat(kKbGroupArrows, kb_Left) && s.j > 0)
						s.j--;
				if (input_.repeat(kKbGroupArrows, kb_Right) && (s.j + 1u) < s.n)
						s.j++;
		}

		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		if (s.slot >= kSlotCount || !slots_[s.slot].is_set())
				fail_fast("update_cofactor_element: expected slot to be set");

		const matrix_core::MatrixView a = slots_[s.slot].view_active();
		SHELL_DBG("[cof] RUN slot=%c i=%u j=%u\n", (char)('A' + s.slot), (unsigned)s.i, (unsigned)s.j);

		// ephemeral tail has to be clear before allocating an explanation
		persist_.rewind(persist_base_mark_);
		persist_tail_mark_ = persist_base_mark_;
		expl_ = {};

		matrix_core::ArenaScope tx(persist_);
		matrix_core::ArenaScratchScope scratch_tx(scratch_);
		matrix_core::ExplainOptions opts_steps;
		opts_steps.enable = true;
		opts_steps.persist = &persist_;

		matrix_core::Explanation expl;
		matrix_core::Rational cofactor = matrix_core::Rational::from_int(0);
		const matrix_core::Error err = matrix_core::op_cofactor_element(a, s.i, s.j, scratch_, &cofactor, &expl, opts_steps);

		SHELL_DBG("[op] cofactor err=%u a=%ux%u\n", (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
		if (!matrix_core::is_ok(err)) {
				if (err.code == matrix_core::ErrorCode::NotSquare || err.code == matrix_core::ErrorCode::IndexOutOfRange ||
				        err.code == matrix_core::ErrorCode::Internal) {
						fail_fast("update_cofactor_element: cofactor returned unexpected error");
				}
				show_message("Error");
				return;
		}

		tx.commit();
		expl_ = std::move(expl);

		// pop Cofactor page and its underlying slot picker, then show result
		if (depth_ < 2)
				fail_fast("update_cofactor_element: expected >=2 pages");
		if (stack_[depth_ - 1].kind != PageKind::CofactorElement)
				fail_fast("update_cofactor_element: expected CofactorElement on top");
		if (stack_[depth_ - 2].kind != PageKind::SlotPick)
				fail_fast("update_cofactor_element: expected SlotPick under CofactorElement");

		REQUIRE(pop(), "pop failed (cofactor page)");
		REQUIRE(pop(), "pop failed (slot pick)");
		REQUIRE(push(Page::make_result_cofactor_element(s.slot, s.i, s.j, expl_.available(), cofactor.num(), cofactor.den())),
		        "push result failed");
}
#else
void App::update_cofactor_element(CofactorElementState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				REQUIRE(pop(), "pop failed");
				return;
		}
		(void)s;
}
#endif

} // namespace matrix_shell
