#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>


namespace matrix_shell {
namespace {
using detail::kKbGroup1;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
using detail::op_name;

void render_pivot_cols(std::uint32_t mask, char* buf, std::size_t cap) noexcept {
		buf[0] = '\0';
		matrix_core::CheckedWriter w{buf, cap};
		bool first = true;
		for (std::uint8_t i = 0; i < 7; i++) {
				if ((mask & (1u << i)) == 0)
						continue;
				if (!first)
						w.put(' ');
				w.append_u64(static_cast<std::uint64_t>(i + 1u));
				first = false;
		}
		if (first)
				w.append("-");
}

void print_rational(matrix_core::Rational value, char* buf, std::size_t cap) noexcept {
		matrix_core::CheckedWriter w{buf, cap};
		w.append_i64(value.num());
		if (value.den() != 1) {
				w.put('/');
				w.append_i64(value.den());
		}
		gfx_PrintString(buf);
}
} // namespace

void App::render_result(const ResultState& s) noexcept {
		const ui::Layout l{};
		if (s.op == OperationId::Projection) {
				render_header(tr(TextId::ResultProjectionTitle));
		} else {
				render_header(nullptr);
				gfx_PrintString(op_name(s.op));
				gfx_PrintString(tr(TextId::ResultSuffix));
		}
		if (s.op == OperationId::SolveRref || s.op == OperationId::Projection)
				render_footer_hint(tr(s.has_steps ? TextId::FooterProjectionStepsBack : TextId::FooterProjectionBack));
		else
				render_footer_hint(tr(s.has_steps ? TextId::FooterResultStepsBack : TextId::FooterClearBack));
		gfx_SetTextFGColor(ui::color::kBlack);

		if (s.op == OperationId::Projection) {
				const auto& result = s.value.projection;
				gfx_SetTextXY(l.margin_x, l.header_h + 4);
				gfx_PrintString(tr(result.show_orthogonal ? TextId::ResultOrthLabel : TextId::ResultProjLabel));
				gfx_PrintString(tr(TextId::ResultProjectionUPrefix));
				gfx_PrintChar(static_cast<char>('A' + s.slot_a));
				gfx_PrintString(tr(TextId::ResultProjectionVPrefix));
				gfx_PrintChar(static_cast<char>('A' + s.slot_b));
				gfx_PrintString(")");
				gfx_SetTextXY(l.margin_x, l.header_h + 18);
				gfx_PrintString(tr(TextId::ResultKLabel));
				print_rational(result.factor, fmt_buf_, sizeof(fmt_buf_));
				render_result_grid(result.show_orthogonal ? result.orthogonal : result.projected, l.header_h + 34);
				return;
		}

		if (s.op == OperationId::SpanTest || s.op == OperationId::IndepTest) {
				const auto& result = s.value.predicate;
				const int y = l.header_h + 26;
				gfx_SetTextXY(l.margin_x, y);
				gfx_PrintString(s.op == OperationId::SpanTest ? tr(TextId::ResultSpanLabel) : tr(TextId::ResultIndependentLabel));
				gfx_PrintString(result.value ? tr(TextId::ResultBoolYes) : tr(TextId::ResultBoolNo));
				gfx_SetTextXY(l.margin_x, y + 14);
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append(tr(TextId::ResultRankEq));
				w.append_u64(result.space.rank);
				w.append(tr(TextId::ResultMEq));
				w.append_u64(result.input.rows);
				w.append(tr(TextId::ResultNEq));
				w.append_u64(result.input.cols);
				gfx_PrintString(fmt_buf_);
				gfx_SetTextXY(l.margin_x, y + 28);
				gfx_PrintString(tr(TextId::ResultPivEq));
				render_pivot_cols(result.space.pivot_mask, fmt_buf_, sizeof(fmt_buf_));
				gfx_PrintString(fmt_buf_);
				return;
		}

		if (s.op == OperationId::Det || s.op == OperationId::Dot || s.op == OperationId::CofactorElement) {
				gfx_SetTextXY(l.margin_x, l.header_h + 30);
				if (s.op == OperationId::CofactorElement) {
						const auto& result = s.value.cofactor;
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append("C_{");
						w.append_index1(result.row);
						w.put(',');
						w.append_index1(result.col);
						w.append("} = ");
						gfx_PrintString(fmt_buf_);
						print_rational(result.value, fmt_buf_, sizeof(fmt_buf_));
				} else {
						gfx_PrintString(s.op == OperationId::Det ? "det(" : "dot(");
						gfx_PrintChar(static_cast<char>('A' + s.slot_a));
						if (s.op == OperationId::Dot) {
								gfx_PrintString(", ");
								gfx_PrintChar(static_cast<char>('A' + s.slot_b));
						}
						gfx_PrintString(") = ");
						print_rational(s.value.scalar, fmt_buf_, sizeof(fmt_buf_));
				}
				return;
		}

		matrix_core::MatrixView matrix;
		const SpaceSummary* space = nullptr;
		if (s.op == OperationId::SolveRref) {
				const auto& result = s.value.solution;
				matrix = result.show_null_basis ? result.null_basis : result.particular;
				space = &result.space;
		} else if (s.op == OperationId::ColSpaceBasis || s.op == OperationId::RowSpaceBasis ||
		        s.op == OperationId::NullSpaceBasis || s.op == OperationId::LeftNullSpaceBasis) {
				matrix = s.value.basis.vectors;
				space = &s.value.basis.space;
		} else {
				matrix = s.value.matrix;
		}

		int grid_top = l.header_h + 8;
		if (space) {
				gfx_SetTextXY(l.margin_x, l.header_h + 4);
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append(tr(TextId::ResultRankEq));
				w.append_u64(space->rank);
				w.append(tr(TextId::ResultNullEq));
				w.append_u64(space->nullity);
				w.append(tr(TextId::ResultPivCompactEq));
				gfx_PrintString(fmt_buf_);
				render_pivot_cols(space->pivot_mask, fmt_buf_, sizeof(fmt_buf_));
				gfx_PrintString(fmt_buf_);
				gfx_SetTextXY(l.margin_x, l.header_h + 16);
				if (s.op == OperationId::SolveRref)
						gfx_PrintString(s.value.solution.show_null_basis ? tr(TextId::ResultSolutionBasis) : tr(TextId::ResultParticularSolution));
				else
						gfx_PrintString(s.op == OperationId::RowSpaceBasis ? tr(TextId::ResultRowsBasis) : tr(TextId::ResultColsBasis));
				grid_top = l.header_h + 30;
		}
		render_result_grid(matrix, grid_top);
}

void App::render_result_grid(matrix_core::MatrixView matrix, int top) noexcept {
		const ui::Layout l{};
		if (matrix.rows == 0 || matrix.cols == 0) {
				gfx_SetTextXY(l.margin_x, top);
				gfx_PrintString(tr(TextId::ResultEmptyBasis));
				return;
		}
		const int cell_w = (kScreenW - 2 * l.margin_x) / 6;
		const int cell_h = (kScreenH - l.footer_h - l.margin_y - top) / 6;
		for (std::uint8_t r = 0; r < matrix.rows; r++) {
				for (std::uint8_t c = 0; c < matrix.cols; c++) {
						const int x = l.margin_x + c * cell_w;
						const int y = top + r * cell_h;
						gfx_SetColor(ui::color::kLightGray);
						gfx_FillRectangle(x, y, cell_w - 1, cell_h - 1);
						gfx_SetColor(ui::color::kDarkGray);
						gfx_Rectangle(x, y, cell_w - 1, cell_h - 1);
						gfx_SetTextFGColor(ui::color::kBlack);
						gfx_SetTextXY(x + 2, y + 3);
						print_rational(matrix.at(r, c), fmt_buf_, sizeof(fmt_buf_));
				}
		}
}

void App::update_result(ResultState& s) noexcept {
		if (s.has_steps && input_.pressed(kKbGroup1, kb_2nd)) {
				if (s.op == OperationId::Cramer)
						push(Page::make_cramer_steps_menu(s.slot_a, s.slot_b, s.value.matrix.rows));
				else
						push(Page::make_steps());
				return;
		}
		if ((s.op == OperationId::SolveRref || s.op == OperationId::Projection) &&
		        (input_.repeat(kKbGroupArrows, kb_Left) || input_.repeat(kKbGroupArrows, kb_Right))) {
				bool& toggle = s.op == OperationId::SolveRref ? s.value.solution.show_null_basis : s.value.projection.show_orthogonal;
				toggle = !toggle;
				return;
		}

		if (!input_.pressed(kKbGroup6, kb_Clear))
				return;
		SHELL_DBG("[result] CLEAR back\n");
		pop();
}

} // namespace matrix_shell
