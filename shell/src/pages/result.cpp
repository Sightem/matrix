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
using detail::kKbGroup1;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
using detail::op_name;

void render_pivot_cols(std::uint32_t mask, char* buf, std::size_t cap) noexcept {
		if (!buf || cap == 0)
				return;
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
} // namespace

void App::render_result(const ResultState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header(nullptr);
		gfx_PrintString(op_name(s.op));
		gfx_PrintString(" Result");
		if (s.has_steps)
				render_footer_hint("2ND: Steps  CLEAR: Back");
		else
				render_footer_hint("CLEAR: Back");

		gfx_SetTextFGColor(ui::color::kBlack);

		if (s.is_scalar) {
				if (s.op == OperationId::SpanTest || s.op == OperationId::IndepTest) {
						const int y0 = l.header_h + 26;
						gfx_SetTextXY(l.margin_x, y0);
						gfx_PrintString((s.op == OperationId::SpanTest) ? "Spans R^m:" : "Independent:");
						gfx_PrintString((s.num != 0) ? " YES" : " NO");

						gfx_SetTextXY(l.margin_x, y0 + 14);
						gfx_PrintString("rank=");
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append_u64(static_cast<std::uint64_t>(s.i));
						gfx_PrintString(fmt_buf_);

						gfx_PrintString("  m=");
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w2{fmt_buf_, sizeof(fmt_buf_)};
						w2.append_u64(static_cast<std::uint64_t>(s.rows));
						gfx_PrintString(fmt_buf_);
						gfx_PrintString("  n=");
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w3{fmt_buf_, sizeof(fmt_buf_)};
						w3.append_u64(static_cast<std::uint64_t>(s.cols));
						gfx_PrintString(fmt_buf_);

						gfx_SetTextXY(l.margin_x, y0 + 28);
						gfx_PrintString("piv: ");
						render_pivot_cols(static_cast<std::uint32_t>(s.den), fmt_buf_, sizeof(fmt_buf_));
						gfx_PrintString(fmt_buf_);
						return;
				}

				if (s.op == OperationId::Det) {
						gfx_SetTextXY(l.margin_x, l.header_h + 30);
						gfx_PrintString("det(");
						gfx_PrintChar(static_cast<char>('A' + s.slot_a));
						gfx_PrintString(") = ");
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						if (s.den == 1) {
								w.append_i64(s.num);
						} else {
								w.append_i64(s.num);
								w.put('/');
								w.append_i64(s.den);
						}
						gfx_PrintString(fmt_buf_);
						return;
				}

				if (s.op == OperationId::Dot) {
						gfx_SetTextXY(l.margin_x, l.header_h + 30);
						gfx_PrintString("dot(");
						gfx_PrintChar(static_cast<char>('A' + s.slot_a));
						gfx_PrintString(", ");
						gfx_PrintChar(static_cast<char>('A' + s.slot_b));
						gfx_PrintString(") = ");

						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						if (s.den == 1)
								w.append_i64(s.num);
						else {
								w.append_i64(s.num);
								w.put('/');
								w.append_i64(s.den);
						}
						gfx_PrintString(fmt_buf_);
						return;
				}

#if MATRIX_SHELL_ENABLE_COFACTOR
				if (s.op == OperationId::CofactorElement) {
						const int y0 = l.header_h + 30;

						gfx_SetTextXY(l.margin_x, y0);
						gfx_PrintString("C_{");

						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append_u64(static_cast<std::uint64_t>(s.i) + 1u);
						w.put(',');
						w.append_u64(static_cast<std::uint64_t>(s.j) + 1u);
						gfx_PrintString(fmt_buf_);
						gfx_PrintString("} = ");

						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w2{fmt_buf_, sizeof(fmt_buf_)};
						if (s.den == 1)
								w2.append_i64(s.num);
						else {
								w2.append_i64(s.num);
								w2.put('/');
								w2.append_i64(s.den);
						}
						gfx_PrintString(fmt_buf_);
						return;
				}
#endif

				// scalar result op not handled should not happen
				SHELL_DBG("[result] unsupported scalar op=%s num=%lld den=%lld\n", op_name(s.op), (long long)s.num, (long long)s.den);
				gfx_SetTextXY(l.margin_x, l.header_h + 30);
				gfx_PrintString("Internal error: unsupported scalar op");
				return;
		}

		if (!s.data || s.rows < 1 || s.cols < 1 || s.stride < s.cols)
				fail_fast("render_result: invalid matrix result state");

		int grid_top = l.header_h + 8;
		if (s.op == OperationId::SolveRref || s.op == OperationId::ColSpaceBasis || s.op == OperationId::RowSpaceBasis ||
		        s.op == OperationId::NullSpaceBasis || s.op == OperationId::LeftNullSpaceBasis) {
				gfx_SetTextXY(l.margin_x, l.header_h + 4);
				gfx_PrintString("rank=");
				fmt_buf_[0] = '\0';
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append_u64(static_cast<std::uint64_t>(s.i));
				gfx_PrintString(fmt_buf_);

				gfx_PrintString(" null=");
				fmt_buf_[0] = '\0';
				matrix_core::CheckedWriter w2{fmt_buf_, sizeof(fmt_buf_)};
				w2.append_u64(static_cast<std::uint64_t>(s.j));
				gfx_PrintString(fmt_buf_);

				gfx_PrintString(" piv:");
				render_pivot_cols(static_cast<std::uint32_t>(s.num), fmt_buf_, sizeof(fmt_buf_));
				gfx_PrintString(fmt_buf_);

				gfx_SetTextXY(l.margin_x, l.header_h + 16);
				if (s.op == OperationId::RowSpaceBasis) {
						gfx_PrintString("Rows are basis vectors");
				} else if (s.op == OperationId::SolveRref) {
						if (s.cols == 1) {
								gfx_PrintString("x is the solution vector");
						} else {
								gfx_PrintString("col1=x_p, cols2+=Null(A)");
						}
				} else {
						gfx_PrintString("Columns are basis vectors");
				}
				grid_top = l.header_h + 30;
		}

		const int grid_left = l.margin_x;
		const int max_cols = 6;
		const int max_rows = 6;

		const int cell_w = (kScreenW - 2 * l.margin_x) / max_cols;
		const int cell_h = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / max_rows;

		const std::uint8_t rows = s.rows;
		const std::uint8_t cols = s.cols;

		for (std::uint8_t r = 0; r < rows; r++) {
				for (std::uint8_t c = 0; c < cols; c++) {
						const int x = grid_left + c * cell_w;
						const int y = grid_top + r * cell_h;

						gfx_SetColor(ui::color::kLightGray);
						gfx_FillRectangle(x, y, cell_w - 1, cell_h - 1);
						gfx_SetColor(ui::color::kDarkGray);
						gfx_Rectangle(x, y, cell_w - 1, cell_h - 1);

						const matrix_core::Rational& v = s.data[static_cast<std::size_t>(r) * s.stride + static_cast<std::size_t>(c)];
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						if (v.den() == 1) {
								w.append_i64(v.num());
						} else {
								w.append_i64(v.num());
								w.put('/');
								w.append_i64(v.den());
						}

						gfx_SetTextFGColor(ui::color::kBlack);
						gfx_SetTextXY(x + 2, y + 3);
						gfx_PrintString(fmt_buf_);
				}
		}
}

void App::update_result(ResultState& s) noexcept {
		// steps (cramer uses a selector menu, others go straight to pager)
#if MATRIX_SHELL_ENABLE_CRAMER
		if (s.op == OperationId::Cramer && s.has_steps && input_.pressed(kKbGroup1, kb_2nd)) {
				SHELL_DBG("[result] 2ND cramer steps menu\n");
				REQUIRE(push(Page::make_cramer_steps_menu(s.slot_a, s.slot_b, s.rows)), "push cramer steps menu failed");
				return;
		}
#endif
		if (s.has_steps && input_.pressed(kKbGroup1, kb_2nd) && expl_.available()) {
				SHELL_DBG("[result] 2ND steps\n");
				REQUIRE(push(Page::make_steps()), "push steps failed");
				return;
		}

		if (!input_.pressed(kKbGroup6, kb_Clear))
				return;
		SHELL_DBG("[result] CLEAR back -> drop tail\n");
		REQUIRE(pop(), "pop failed");
		// drop ephemeral result buffers
		persist_.rewind(persist_base_mark_);
		persist_tail_mark_ = persist_base_mark_;
		expl_ = {};
}

void App::render_projection_result(const ProjectionResultState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header("Projection");
		if (s.has_steps)
				render_footer_hint("LEFT/RIGHT: Toggle  2ND: Steps  CLEAR: Back");
		else
				render_footer_hint("LEFT/RIGHT: Toggle  CLEAR: Back");

		const bool show_proj = (s.mode == 0);
		if (s.rows < 1 || s.cols < 1)
				fail_fast("render_projection_result: invalid dims");
		if (show_proj ? (s.proj_data == nullptr) : (s.orth_data == nullptr))
				fail_fast("render_projection_result: missing vector data");
		const matrix_core::MatrixView vec = show_proj ? matrix_core::MatrixView{s.rows, s.cols, s.stride_proj, s.proj_data}
		                                              : matrix_core::MatrixView{s.rows, s.cols, s.stride_orth, s.orth_data};
		if (vec.stride < vec.cols)
				fail_fast("render_projection_result: invalid stride");

		gfx_SetTextFGColor(ui::color::kBlack);

		// info line
		gfx_SetTextXY(l.margin_x, l.header_h + 4);
		gfx_PrintString(show_proj ? "proj" : "orth");
		gfx_PrintString(" (u=");
		gfx_PrintChar(static_cast<char>('A' + s.slot_u));
		gfx_PrintString(", v=");
		gfx_PrintChar(static_cast<char>('A' + s.slot_v));
		gfx_PrintString(")");

		// k line
		gfx_SetTextXY(l.margin_x, l.header_h + 18);
		gfx_PrintString("k: ");
		fmt_buf_[0] = '\0';
		{
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				if (s.k_den == 1) {
						w.append_i64(s.k_num);
				} else {
						w.append_i64(s.k_num);
						w.put('/');
						w.append_i64(s.k_den);
				}
		}
		gfx_PrintString(fmt_buf_);

		// vector grid
		const int grid_top = l.header_h + 34;
		const int grid_left = l.margin_x;
		const int max_cols = 6;
		const int max_rows = 6;

		const int cell_w = (kScreenW - 2 * l.margin_x) / max_cols;
		const int cell_h = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / max_rows;

		const std::uint8_t rows = vec.rows;
		const std::uint8_t cols = vec.cols;

		for (std::uint8_t r = 0; r < rows; r++) {
				for (std::uint8_t c = 0; c < cols; c++) {
						const int x = grid_left + c * cell_w;
						const int y = grid_top + r * cell_h;

						gfx_SetColor(ui::color::kLightGray);
						gfx_FillRectangle(x, y, cell_w - 1, cell_h - 1);
						gfx_SetColor(ui::color::kDarkGray);
						gfx_Rectangle(x, y, cell_w - 1, cell_h - 1);

						const matrix_core::Rational& v = vec.data[static_cast<std::size_t>(r) * vec.stride + static_cast<std::size_t>(c)];
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						if (v.den() == 1) {
								w.append_i64(v.num());
						} else {
								w.append_i64(v.num());
								w.put('/');
								w.append_i64(v.den());
						}

						gfx_SetTextFGColor(ui::color::kBlack);
						gfx_SetTextXY(x + 2, y + 3);
						gfx_PrintString(fmt_buf_);
				}
		}
}

void App::update_projection_result(ProjectionResultState& s) noexcept {
		// steps
		if (s.has_steps && input_.pressed(kKbGroup1, kb_2nd) && expl_.available()) {
				SHELL_DBG("[proj] 2ND steps\n");
				REQUIRE(push(Page::make_steps()), "push steps failed");
				return;
		}

		// toggle view
		if (input_.repeat(kKbGroupArrows, kb_Left) || input_.repeat(kKbGroupArrows, kb_Right)) {
				s.mode = (s.mode == 0) ? 1 : 0;
				SHELL_DBG("[proj] toggle mode=%u\n", (unsigned)s.mode);
				return;
		}

		if (!input_.pressed(kKbGroup6, kb_Clear))
				return;

		SHELL_DBG("[proj] CLEAR back -> drop tail\n");
		REQUIRE(pop(), "pop failed");
		persist_.rewind(persist_base_mark_);
		persist_tail_mark_ = persist_base_mark_;
		expl_ = {};
}

} // namespace matrix_shell
