#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
} // namespace

void App::render_dim(const DimState& s) noexcept {
		render_header(nullptr);
		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintString(" Size");
		render_footer_hint("ARROWS: Adjust  ENTER: OK  CLEAR: Back");

		const ui::Layout l = ui::Layout{};
		const int y = l.header_h + 40;

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, y);
		gfx_PrintString("Rows:");
		gfx_SetTextXY(l.margin_x + 60, y);
		gfx_PrintChar(static_cast<char>('0' + s.rows));

		gfx_SetTextXY(l.margin_x, y + 18);
		gfx_PrintString("Cols:");
		gfx_SetTextXY(l.margin_x + 60, y + 18);
		gfx_PrintChar(static_cast<char>('0' + s.cols));
}

void App::update_dim(DimState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[dim] CLEAR back slot=%c\n", (char)('A' + s.slot));
				REQUIRE(pop(), "pop failed");
				return;
		}

		if (input_.repeat(kKbGroupArrows, kb_Left)) {
				if (s.rows > 1)
						s.rows--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Right)) {
				if (s.rows < 6)
						s.rows++;
		}
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.cols < 6)
						s.cols++;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.cols > 1)
						s.cols--;
		}

		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		SHELL_DBG("[dim] ENTER slot=%c rows=%u cols=%u\n", (char)('A' + s.slot), (unsigned)s.rows, (unsigned)s.cols);

		const matrix_core::ErrorCode ec = ensure_slot_allocated(s.slot);
		if (!matrix_core::is_ok(ec)) {
				if (ec == matrix_core::ErrorCode::Overflow) {
						show_message("Out of memory");
						return;
				}
				fail_fast("ensure_slot_allocated: unexpected failure");
		}

		// resize clear full backing store
		persist_.rewind(persist_base_mark_);
		persist_tail_mark_ = persist_base_mark_;
		expl_ = {};
		matrix_core::matrix_fill_zero(slots_[s.slot].backing);
		slots_[s.slot].rows = s.rows;
		slots_[s.slot].cols = s.cols;
		SHELL_DBG("[dim] resized %c active=%ux%u backing=%p base_mark=%u used=%u/%u\n",
		        (char)('A' + s.slot),
		        (unsigned)s.rows,
		        (unsigned)s.cols,
		        (void*)slots_[s.slot].backing.data,
		        (unsigned)persist_base_mark_,
		        (unsigned)persist_.used(),
		        (unsigned)persist_.capacity());

		// replace this page with the editor so CLEAR from editor returns to slot list
		REQUIRE(pop(), "pop failed");
		REQUIRE(push(Page::make_editor(s.slot)), "push editor failed");
}

} // namespace matrix_shell
