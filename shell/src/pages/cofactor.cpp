#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>


namespace matrix_shell {
namespace {
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenW;
} // namespace

#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
void App::render_cofactor_element(const CofactorElementState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header(tr(TextId::OpCofactor));
		render_footer_hint(tr(TextId::FooterCofactorAdjust));

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 10);
		gfx_PrintString(tr(TextId::CofactorMatrixPrefix));
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
		draw_field(y_base + 0 * line_h, tr(TextId::CofactorRowI), fmt_buf_, s.focus == 0);

		fmt_buf_[0] = '\0';
		{
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append_u64(static_cast<std::uint64_t>(s.j) + 1u);
		}
		draw_field(y_base + 1 * line_h, tr(TextId::CofactorColJ), fmt_buf_, s.focus == 1);
}
#else
void App::render_cofactor_element(const CofactorElementState& s) noexcept {
		const ui::Layout l = ui::Layout{};
		render_header(tr(TextId::OpCofactor));
		render_footer_hint(tr(TextId::FooterClearBack));
		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(l.margin_x, l.header_h + 30);
		gfx_PrintString(tr(TextId::CommonDisabledBuild));
		(void)s;
}
#endif

#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
void App::update_cofactor_element(CofactorElementState& s) noexcept {
		// back
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[cof] CLEAR back\n");
				pop();
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


		execute_operation(OperationId::CofactorElement, s.slot, 0, s.i, s.j);
}
#else
void App::update_cofactor_element(CofactorElementState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				pop();
				return;
		}
		(void)s;
}
#endif

} // namespace matrix_shell
