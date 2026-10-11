#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
} // namespace

void App::render_dim(const DimState& s) noexcept {
		render_header(nullptr);
		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintString(tr(TextId::DimTitleSuffix));
		render_footer_hint(tr(TextId::FooterDim));

		const ui::Layout l = ui::Layout{};
		const int y = l.header_h + 40;

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, y);
		gfx_PrintString(tr(TextId::DimRowsLabel));
		gfx_SetTextXY(l.margin_x + 60, y);
		gfx_PrintChar(static_cast<char>('0' + s.rows));

		gfx_SetTextXY(l.margin_x, y + 18);
		gfx_PrintString(tr(TextId::DimColsLabel));
		gfx_SetTextXY(l.margin_x + 60, y + 18);
		gfx_PrintChar(static_cast<char>('0' + s.cols));
}

void App::update_dim(DimState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[dim] CLEAR back slot=%c\n", (char)('A' + s.slot));
				pop();
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

		calculation_.explanation = {};
		matrix_core::matrix_fill_zero({6, 6, 6, slots_[s.slot].data});
		slots_[s.slot].rows = s.rows;
		slots_[s.slot].cols = s.cols;
		SHELL_DBG("[dim] resized %c active=%ux%u\n", (char)('A' + s.slot), (unsigned)s.rows, (unsigned)s.cols);

		// replace this page with the editor so CLEAR from editor returns to slot list
		pop();
		push(Page::make_editor(s.slot));
}

} // namespace matrix_shell
