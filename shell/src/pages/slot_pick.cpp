#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/spaces.hpp"
#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>

#include <new>
#include <utility>

namespace matrix_shell {
namespace {
using detail::dbg_print_matrix;
using detail::dbg_print_rational;
using detail::fail_fast;
using detail::keep_cursor_in_view;
using detail::kKbGroup1;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
using detail::op_is_binary;
using detail::op_name;
using namespace matrix_shell::text_literals;
} // namespace

void App::render_slot_pick(const SlotPickState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		render_header(nullptr);
		gfx_PrintString(op_name(s.op));
		gfx_PrintString(": ");
		if (op_is_binary(s.op))
				gfx_PrintString((s.stage == 0) ? "slot_pick.pick1"_tx : "slot_pick.pick2"_tx);
		else
				gfx_PrintString("slot_pick.pick"_tx);

		render_footer_hint("footer.select_back"_tx);

		const int list_top = l.header_h + l.margin_y;
		const int line_h = 14;
		const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;

		for (std::uint8_t row = 0; row < visible; row++) {
				const std::uint8_t idx = static_cast<std::uint8_t>(s.scroll + row);
				if (idx >= kSlotCount)
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

				const char name = static_cast<char>('A' + idx);
				gfx_PrintChar(name);
				gfx_PrintString(": ");

				const Slot& slot = slots_[idx];
				if (!slot.is_set()) {
						gfx_PrintString("common.unset"_tx);
				} else {
						gfx_PrintChar(static_cast<char>('0' + slot.rows));
						gfx_PrintChar('x');
						gfx_PrintChar(static_cast<char>('0' + slot.cols));
				}
		}
}

void App::update_slot_pick(SlotPickState& s) noexcept {
		// back
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				if (op_is_binary(s.op) && s.stage == 1) {
						SHELL_DBG("[pick] CLEAR backstep op=%s stage=1 -> 0\n", op_name(s.op));
						s.stage = 0;
						return;
				}
				SHELL_DBG("[pick] CLEAR pop op=%s stage=%u\n", op_name(s.op), (unsigned)s.stage);
				REQUIRE(pop(), "pop failed");
				return;
		}

		const std::uint8_t old_cursor = s.cursor;
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.cursor > 0)
						s.cursor--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.cursor + 1 < kSlotCount)
						s.cursor++;
		}
		if (s.cursor != old_cursor) {
				const int line_h = 14;
				const ui::Layout l = ui::Layout{};
				const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;
				s.scroll = keep_cursor_in_view(s.cursor, s.scroll, kSlotCount, visible);
		}

		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		const std::uint8_t sel = s.cursor;
		if (sel >= kSlotCount || !slots_[sel].is_set()) {
				SHELL_DBG("[pick] ENTER sel=%c unset\n", (char)('A' + sel));
				show_message("common.slot_unset"_tx);
				return;
		}

		// drop any previous ephemeral result
		persist_.rewind(persist_base_mark_);
		expl_ = {};
		persist_tail_mark_ = persist_base_mark_;
		SHELL_DBG("[pick] rewind persist to base_mark=%u used=%u/%u\n",
		        (unsigned)persist_base_mark_,
		        (unsigned)persist_.used(),
		        (unsigned)persist_.capacity());

		// steps are enabled per operation below when supported

		if (!op_is_binary(s.op)) {
				const matrix_core::MatrixView a = slots_[sel].view_active();
				SHELL_DBG("[pick] unary op=%s sel=%c\n", op_name(s.op), (char)('A' + sel));
				dbg_print_matrix("A", a);

#if !MATRIX_SHELL_ENABLE_MINOR_MATRIX
				if (s.op == OperationId::MinorMatrix) {
						show_message("common.disabled_build_short"_tx);
						return;
				}
#endif
#if !MATRIX_SHELL_ENABLE_COFACTOR
				if (s.op == OperationId::CofactorElement) {
						show_message("common.disabled_build_short"_tx);
						return;
				}
#endif

#if MATRIX_SHELL_ENABLE_MINOR_MATRIX && MATRIX_CORE_ENABLE_MINOR_MATRIX
				if (s.op == OperationId::MinorMatrix) {
						if (a.rows != a.cols) {
								fmt_buf_[0] = '\0';
								matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
								w.append("msg.requires_square_prefix"_tx);
								w.put(static_cast<char>('A' + sel));
								w.append("=");
								w.append_u64(a.rows);
								w.put('x');
								w.append_u64(a.cols);
								show_message(fmt_buf_);
								return;
						}
						if (a.rows <= 1) {
								show_message("msg.need_n_ge_2"_tx);
								return;
						}

						matrix_core::ArenaScope tx(persist_);
						matrix_core::MatrixMutView out{};
						const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, a.rows, a.cols, &out);
						if (!matrix_core::is_ok(ec)) {
								SHELL_DBG("[op] minor matrix alloc failed ec=%u used=%u/%u\n",
								        (unsigned)ec,
								        (unsigned)persist_.used(),
								        (unsigned)persist_.capacity());
								show_message("common.out_of_memory"_tx);
								return;
						}

						matrix_core::ArenaScratchScope scratch_tx(scratch_);
						const matrix_core::Error err = matrix_core::op_minor_matrix(a, scratch_, out);
						SHELL_DBG("[op] minor matrix err=%u a=%ux%u\n", (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
						if (!matrix_core::is_ok(err)) {
								if (err.code == matrix_core::ErrorCode::NotSquare || err.code == matrix_core::ErrorCode::InvalidDimension ||
								        err.code == matrix_core::ErrorCode::Internal) {
										fail_fast("update_slot_pick: minor matrix returned unexpected error");
								}
								show_message("common.error"_tx);
								return;
						}

						tx.commit();
						expl_ = {};
						REQUIRE(pop(), "pop failed (minor matrix)");
						REQUIRE(push(Page::make_result_matrix(
						                OperationId::MinorMatrix, sel, 0, false, out.rows, out.cols, out.stride, out.data)),
						        "push minor matrix result failed");
						return;
				}
#endif

#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
				if (s.op == OperationId::CofactorElement) {
						if (a.rows != a.cols) {
								fmt_buf_[0] = '\0';
								matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
								w.append("msg.requires_square_prefix"_tx);
								w.put(static_cast<char>('A' + sel));
								w.append("=");
								w.append_u64(a.rows);
								w.put('x');
								w.append_u64(a.cols);
								show_message(fmt_buf_);
								return;
						}

						REQUIRE(push(Page::make_cofactor_element(sel, a.rows)), "push cofactor element page failed");
						return;
				}
#endif

				if (s.op == OperationId::Transpose) {
						matrix_core::ArenaScope tx(persist_);
						matrix_core::MatrixMutView out{};
						const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, a.cols, a.rows, &out);
						if (!matrix_core::is_ok(ec)) {
								SHELL_DBG("[op] transpose alloc failed ec=%u used=%u/%u\n",
								        (unsigned)ec,
								        (unsigned)persist_.used(),
								        (unsigned)persist_.capacity());
								show_message("common.out_of_memory"_tx);
								return;
						}

						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;

						matrix_core::Explanation expl;
						const matrix_core::Error err = matrix_core::op_transpose(a, out, &expl, opts_steps);
						SHELL_DBG("[op] transpose err=%u a=%ux%u\n", (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
						if (!matrix_core::is_ok(err)) {
								if (err.code == matrix_core::ErrorCode::Overflow) {
										show_message("common.out_of_memory"_tx);
										return;
								}
								fail_fast("update_slot_pick: transpose returned unexpected error");
						}

						tx.commit();
						expl_ = std::move(expl);
						REQUIRE(pop(), "pop failed (transpose)");
						REQUIRE(push(Page::make_result_matrix(
						                OperationId::Transpose, sel, 0, expl_.available(), out.rows, out.cols, out.stride, out.data)),
						        "push transpose result failed");
						return;
				}

				if (s.op == OperationId::Det) {
						matrix_core::ArenaScope tx(persist_);
						matrix_core::ArenaScratchScope scratch_tx(scratch_);

						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;

						matrix_core::Explanation expl;
						matrix_core::Rational v = matrix_core::Rational::from_int(0);
						const matrix_core::Error err = matrix_core::op_det(a, scratch_, &v, &expl, opts_steps);
						SHELL_DBG("[op] det err=%u a=%ux%u value=", (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
						dbg_print_rational(v);
						SHELL_DBG("\n");
						if (!matrix_core::is_ok(err)) {
								if (err.code == matrix_core::ErrorCode::NotSquare) {
										fmt_buf_[0] = '\0';
										matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
										w.append("msg.det_requires_square_prefix"_tx);
										w.put(static_cast<char>('A' + sel));
										w.append("=");
										w.append_u64(a.rows);
										w.put('x');
										w.append_u64(a.cols);
										show_message(fmt_buf_);
								} else
										show_message("common.error"_tx);
								return;
						}

						tx.commit();
						REQUIRE(pop(), "pop failed (det)");
						expl_ = std::move(expl);
						REQUIRE(push(Page::make_result_scalar(OperationId::Det, sel, 0, expl_.available(), v.num(), v.den())),
						        "push det result failed");
						return;
				}

				if (s.op == OperationId::Inverse) {
						if (a.rows != a.cols) {
								fmt_buf_[0] = '\0';
								matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
								w.append("msg.inverse_requires_square_prefix"_tx);
								w.put(static_cast<char>('A' + sel));
								w.append("=");
								w.append_u64(a.rows);
								w.put('x');
								w.append_u64(a.cols);
								show_message(fmt_buf_);
								return;
						}

						matrix_core::ArenaScratchScope scratch_tx(scratch_);

						const std::uint8_t n = a.rows;
						matrix_core::ArenaScope tx(persist_);
						matrix_core::MatrixMutView outm{};
						const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, n, n, &outm);
						if (!matrix_core::is_ok(ec)) {
								show_message("common.out_of_memory"_tx);
								return;
						}

						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;

						matrix_core::Explanation expl;
						const matrix_core::Error err = matrix_core::op_inverse(a, scratch_, outm, &expl, opts_steps);
						SHELL_DBG("[op] inverse err=%u a=%ux%u\n", (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
						if (!matrix_core::is_ok(err)) {
								if (err.code == matrix_core::ErrorCode::Singular) {
										show_message("msg.singular_no_inverse"_tx);
								} else if (err.code == matrix_core::ErrorCode::NotSquare ||
								           err.code == matrix_core::ErrorCode::DimensionMismatch ||
								           err.code == matrix_core::ErrorCode::Internal) {
										fail_fast("update_slot_pick: inverse returned unexpected error");
								} else {
										show_message("common.error"_tx);
								}
								return;
						}

						tx.commit();
						expl_ = std::move(expl);
						REQUIRE(pop(), "pop failed (inverse)");
						REQUIRE(push(Page::make_result_matrix(
						                s.op, sel, 0, expl_.available(), outm.rows, outm.cols, outm.stride, outm.data)),
						        "push inverse result failed");
						return;
				}

				if (s.op == OperationId::SpanTest || s.op == OperationId::IndepTest) {
						matrix_core::ArenaScope tx(persist_);
						matrix_core::ArenaScratchScope scratch_tx(scratch_);

						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;
						matrix_core::Explanation expl;

						matrix_core::MatrixMutView rref{};
						const matrix_core::ErrorCode ec_r = matrix_core::matrix_alloc(scratch_, a.rows, a.cols, &rref);
						if (!matrix_core::is_ok(ec_r)) {
								show_message("common.out_of_memory"_tx);
								return;
						}

						const matrix_core::Error err = matrix_core::op_echelon(a, matrix_core::EchelonKind::Rref, rref, &expl, opts_steps);
						if (!matrix_core::is_ok(err)) {
								show_message("common.error"_tx);
								return;
						}

						matrix_core::SpaceInfo info{};
						const matrix_core::ErrorCode ec_p = matrix_core::space_info_from_rref(rref.view(), a.cols, &info);
						if (!matrix_core::is_ok(ec_p)) {
								show_message("common.error"_tx);
								return;
						}
						const std::uint32_t piv_mask = info.pivot_mask;

						bool ok = false;
						if (s.op == OperationId::SpanTest) {
								ok = (info.rank == a.rows);
						} else {
								ok = (info.rank == a.cols);
						}

						tx.commit();
						expl_ = std::move(expl);
						REQUIRE(pop(), "pop failed (span/indep)");

						Page p = Page::make_result_scalar(s.op, sel, 0, expl_.available(), ok ? 1 : 0, 1);
						p.u.result.rows = a.rows;
						p.u.result.cols = a.cols;
						p.u.result.i = info.rank;    // rank
						p.u.result.j = 0;
						// For span/independence, `num` is the boolean (YES/NO) and `den` stores the pivot mask.
						p.u.result.num = ok ? 1 : 0;
						p.u.result.den = static_cast<std::int64_t>(piv_mask);
						REQUIRE(push(p), "push span/indep result failed");
						return;
				}

				if (s.op == OperationId::ColSpaceBasis || s.op == OperationId::RowSpaceBasis || s.op == OperationId::NullSpaceBasis ||
				        s.op == OperationId::LeftNullSpaceBasis) {
						matrix_core::ArenaScope tx(persist_);
						matrix_core::ArenaScratchScope scratch_tx(scratch_);

						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;
						matrix_core::Explanation expl;

						matrix_core::MatrixView input = a;
						matrix_core::MatrixMutView at_owned{};
						if (s.op == OperationId::LeftNullSpaceBasis) {
								// Keep A^T alive for the step renderer.
								const matrix_core::ErrorCode ec_t = matrix_core::matrix_alloc(persist_, a.cols, a.rows, &at_owned);
								if (!matrix_core::is_ok(ec_t)) {
										show_message("common.out_of_memory"_tx);
										return;
								}
								const matrix_core::ErrorCode ec_tt = matrix_core::matrix_transpose(a, at_owned);
								if (!matrix_core::is_ok(ec_tt)) {
										show_message("common.error"_tx);
										return;
								}
								input = at_owned.view();
						}

						matrix_core::MatrixMutView rref{};
						const matrix_core::ErrorCode ec_r = matrix_core::matrix_alloc(scratch_, input.rows, input.cols, &rref);
						if (!matrix_core::is_ok(ec_r)) {
								show_message("common.out_of_memory"_tx);
								return;
						}

						const matrix_core::Error err = matrix_core::op_echelon(input, matrix_core::EchelonKind::Rref, rref, &expl, opts_steps);
						if (!matrix_core::is_ok(err)) {
								show_message("common.error"_tx);
								return;
						}

						matrix_core::SpaceInfo info{};
						const matrix_core::ErrorCode ec_info = matrix_core::space_info_from_rref(rref.view(), input.cols, &info);
						if (!matrix_core::is_ok(ec_info)) {
								show_message("common.error"_tx);
								return;
						}
						const std::uint32_t piv_mask = info.pivot_mask;
						const std::uint8_t nullity = info.nullity;

						matrix_core::MatrixMutView out{};
						if (s.op == OperationId::ColSpaceBasis) {
								const matrix_core::ErrorCode ec_o = matrix_core::space_col_basis(a, info, persist_, &out);
								if (!matrix_core::is_ok(ec_o)) {
										show_message("common.out_of_memory"_tx);
										return;
								}
						} else if (s.op == OperationId::RowSpaceBasis) {
								const matrix_core::ErrorCode ec_o = matrix_core::space_row_basis(rref.view(), input.cols, info, persist_, &out);
								if (!matrix_core::is_ok(ec_o)) {
										show_message("common.out_of_memory"_tx);
										return;
								}
						} else {
								// Null(A) or Null(A^T)
								const std::uint8_t var_cols = input.cols;
								const matrix_core::ErrorCode ec_o = matrix_core::space_null_basis(rref.view(), var_cols, info, persist_, &out);
								if (!matrix_core::is_ok(ec_o)) {
										show_message("common.out_of_memory"_tx);
										return;
								}
						}

						tx.commit();
						expl_ = std::move(expl);
						Page p = Page::make_result_matrix(s.op, sel, 0, expl_.available(), out.rows, out.cols, out.stride, out.data);
						p.u.result.i = info.rank;    // rank
						p.u.result.j = nullity;     // nullity (in the working variable space)
						p.u.result.num = piv_mask;  // pivot mask (LSB = col 1)
						p.u.result.den = 1;
						REQUIRE(pop(), "pop failed (spaces)");
						REQUIRE(push(p), "push spaces result failed");
						return;
				}

				if (s.op == OperationId::Ref || s.op == OperationId::Rref) {
						matrix_core::ArenaScope tx(persist_);
						matrix_core::MatrixMutView outm{};
						const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, a.rows, a.cols, &outm);
						if (!matrix_core::is_ok(ec)) {
								show_message("common.out_of_memory"_tx);
								return;
						}

						matrix_core::Explanation expl;
						matrix_core::ExplainOptions opts_steps;
						opts_steps.enable = true;
						opts_steps.persist = &persist_;

						const matrix_core::EchelonKind kind =
						        (s.op == OperationId::Ref) ? matrix_core::EchelonKind::Ref : matrix_core::EchelonKind::Rref;
						const matrix_core::Error err = matrix_core::op_echelon(a, kind, outm, &expl, opts_steps);
						SHELL_DBG(
						        "[op] %s err=%u a=%ux%u\n", op_name(s.op), (unsigned)err.code, (unsigned)err.a.rows, (unsigned)err.a.cols);
						if (!matrix_core::is_ok(err)) {
								if (err.code == matrix_core::ErrorCode::DimensionMismatch || err.code == matrix_core::ErrorCode::Internal) {
										fail_fast("update_slot_pick: echelon returned unexpected error");
								}
								show_message("common.error"_tx);
								return;
						}

						tx.commit();
						expl_ = std::move(expl);
						REQUIRE(pop(), "pop failed (echelon)");
						REQUIRE(push(Page::make_result_matrix(
						                s.op, sel, 0, expl_.available(), outm.rows, outm.cols, outm.stride, outm.data)),
						        "push echelon result failed");
						return;
				}

				fail_fast("Unhandled unary OperationId in update_slot_pick");
		}

		// Binary ops: Add/Sub/Mul.
		if (s.stage == 0) {
				SHELL_DBG("[pick] binary op=%s pick1=%c\n", op_name(s.op), (char)('A' + sel));
				s.slot_a = sel;
				s.stage = 1;
				return;
		}

		const std::uint8_t a_slot = s.slot_a;
		const std::uint8_t b_slot = sel;
		const matrix_core::MatrixView a = slots_[a_slot].view_active();
		const matrix_core::MatrixView b = slots_[b_slot].view_active();

		SHELL_DBG("[pick] binary op=%s pick2=%c (A=%c)\n", op_name(s.op), (char)('A' + b_slot), (char)('A' + a_slot));
		dbg_print_matrix("A", a);
		dbg_print_matrix("B", b);

		if (s.op == OperationId::Dot) {
				matrix_core::ArenaScope tx(persist_);
				matrix_core::ArenaScratchScope scratch_tx(scratch_);
				matrix_core::ExplainOptions opts_steps;
				opts_steps.enable = true;
				opts_steps.persist = &persist_;

				matrix_core::Explanation expl;
				matrix_core::Rational v = matrix_core::Rational::from_int(0);
				const matrix_core::Error err = matrix_core::op_dot(a, b, &v, &expl, opts_steps);
				SHELL_DBG("[op] dot err=%u a=%ux%u b=%ux%u value=",
				        (unsigned)err.code,
				        (unsigned)err.a.rows,
				        (unsigned)err.a.cols,
				        (unsigned)err.b.rows,
				        (unsigned)err.b.cols);
				dbg_print_rational(v);
				SHELL_DBG("\n");
				if (!matrix_core::is_ok(err)) {
						if (err.code == matrix_core::ErrorCode::InvalidDimension)
								show_message("msg.need_vectors"_tx);
						else if (err.code == matrix_core::ErrorCode::DimensionMismatch) {
								fmt_buf_[0] = '\0';
								matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
								w.append("msg.need_same_length_prefix"_tx);
								w.put(static_cast<char>('A' + a_slot));
								w.append("=");
								w.append_u64(a.rows);
								w.put('x');
								w.append_u64(a.cols);
								w.append(", ");
								w.put(static_cast<char>('A' + b_slot));
								w.append("=");
								w.append_u64(b.rows);
								w.put('x');
								w.append_u64(b.cols);
								show_message(fmt_buf_);
						} else if (err.code == matrix_core::ErrorCode::Internal) {
								fail_fast("update_slot_pick: dot returned Internal");
						} else
								show_message("common.error"_tx);
						return;
				}

				tx.commit();
				expl_ = std::move(expl);
				REQUIRE(pop(), "pop failed (dot)");
				REQUIRE(push(Page::make_result_scalar(OperationId::Dot, a_slot, b_slot, expl_.available(), v.num(), v.den())),
				        "push dot result failed");
				return;
		}

		if (s.op == OperationId::Cross) {
				matrix_core::ArenaScope tx(persist_);
				matrix_core::MatrixMutView out{};
				const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, a.rows, a.cols, &out);
				if (!matrix_core::is_ok(ec)) {
						SHELL_DBG("[op] cross alloc failed ec=%u used=%u/%u\n",
						        (unsigned)ec,
						        (unsigned)persist_.used(),
						        (unsigned)persist_.capacity());
						show_message("common.out_of_memory"_tx);
						return;
				}

				matrix_core::ExplainOptions opts_steps;
				opts_steps.enable = true;
				opts_steps.persist = &persist_;

				matrix_core::Explanation expl;
				const matrix_core::Error err = matrix_core::op_cross(a, b, out, &expl, opts_steps);
				SHELL_DBG("[op] cross err=%u a=%ux%u b=%ux%u out=%ux%u\n",
				        (unsigned)err.code,
				        (unsigned)err.a.rows,
				        (unsigned)err.a.cols,
				        (unsigned)err.b.rows,
				        (unsigned)err.b.cols,
				        (unsigned)out.rows,
				        (unsigned)out.cols);
				if (!matrix_core::is_ok(err)) {
						if (err.code == matrix_core::ErrorCode::InvalidDimension)
								show_message("msg.need_3d_vectors"_tx);
						else if (err.code == matrix_core::ErrorCode::DimensionMismatch)
								fail_fast("update_slot_pick: cross returned DimensionMismatch");
						else if (err.code == matrix_core::ErrorCode::Internal)
								fail_fast("update_slot_pick: cross returned Internal");
						else
								show_message("common.error"_tx);
						return;
				}

				tx.commit();
				expl_ = std::move(expl);
				REQUIRE(pop(), "pop failed (cross)");
				REQUIRE(push(Page::make_result_matrix(
				                OperationId::Cross, a_slot, b_slot, expl_.available(), out.rows, out.cols, out.stride, out.data)),
				        "push cross result failed");
				return;
		}

#if !MATRIX_SHELL_ENABLE_PROJECTION
		if (s.op == OperationId::Projection) {
				show_message("common.disabled_build_short"_tx);
				return;
		}
#endif
#if !MATRIX_SHELL_ENABLE_CRAMER
		if (s.op == OperationId::Cramer) {
				show_message("common.disabled_build_short"_tx);
				return;
		}
#endif

		if (s.op == OperationId::SolveRref) {
				if (b.cols != 1 || b.rows != a.rows) {
						show_message("msg.need_b_mx1"_tx);
						return;
				}
				if (a.cols + 1 > matrix_core::kMaxCols) {
						show_message("msg.too_many_cols"_tx);
						return;
				}

				matrix_core::ArenaScope tx(persist_);
				matrix_core::ArenaScratchScope scratch_tx(scratch_);

				matrix_core::ExplainOptions opts_steps;
				opts_steps.enable = true;
				opts_steps.persist = &persist_;
				matrix_core::Explanation expl;

				const std::uint8_t m = a.rows;
				const std::uint8_t n = a.cols;

				// Build augmented matrix [A | b] in persist so the step renderer can reference it.
				matrix_core::MatrixMutView aug{};
				const matrix_core::ErrorCode ec_aug = matrix_core::matrix_alloc(persist_, m, static_cast<std::uint8_t>(n + 1u), &aug);
				if (!matrix_core::is_ok(ec_aug)) {
						show_message("common.out_of_memory"_tx);
						return;
				}
				for (std::uint8_t r = 0; r < m; r++) {
						for (std::uint8_t c = 0; c < n; c++)
								aug.at_mut(r, c) = a.at(r, c);
						aug.at_mut(r, n) = b.at(r, 0);
				}

				matrix_core::MatrixMutView rref_aug{};
				const matrix_core::ErrorCode ec_rref = matrix_core::matrix_alloc(scratch_, m, static_cast<std::uint8_t>(n + 1u), &rref_aug);
				if (!matrix_core::is_ok(ec_rref)) {
						show_message("common.out_of_memory"_tx);
						return;
				}

				const matrix_core::Error err = matrix_core::op_echelon(aug.view(), matrix_core::EchelonKind::Rref, rref_aug, &expl, opts_steps);
				if (!matrix_core::is_ok(err)) {
						show_message("common.error"_tx);
						return;
				}

				// Check for inconsistency: [0 ... 0 | c] with c != 0
				for (std::uint8_t r = 0; r < m; r++) {
						bool all_zero = true;
						for (std::uint8_t c = 0; c < n; c++) {
								if (!rref_aug.at(r, c).is_zero()) {
										all_zero = false;
										break;
								}
						}
						if (all_zero && !rref_aug.at(r, n).is_zero()) {
								show_message("msg.no_solution"_tx);
								return;
						}
				}

				matrix_core::SpaceInfo info{};
				const matrix_core::ErrorCode ec_info = matrix_core::space_info_from_rref(rref_aug.view(), n, &info);
				if (!matrix_core::is_ok(ec_info)) {
						show_message("common.error"_tx);
						return;
				}
				const std::uint32_t piv_mask = info.pivot_mask;
				const std::uint8_t nullity = info.nullity;

				// Particular solution: free vars = 0
				matrix_core::MatrixMutView xp{};
				const matrix_core::ErrorCode ec_xp = matrix_core::matrix_alloc(persist_, n, 1, &xp);
				if (!matrix_core::is_ok(ec_xp)) {
						show_message("common.out_of_memory"_tx);
						return;
				}
				matrix_core::matrix_fill_zero(xp);
				for (std::uint8_t pi = 0; pi < info.rank; pi++) {
						const std::uint8_t pc = info.pivot_cols[pi];
						const std::uint8_t pr = info.pivot_row_for_col[pc];
						xp.at_mut(pc, 0) = rref_aug.at(pr, n);
				}

				// Null space basis for A
				matrix_core::MatrixMutView n_basis{};
				const matrix_core::ErrorCode ec_nb = matrix_core::space_null_basis(rref_aug.view(), n, info, persist_, &n_basis);
				if (!matrix_core::is_ok(ec_nb)) {
						show_message("common.out_of_memory"_tx);
						return;
				}

				// Pack output as [x_p | N] when it fits; otherwise show N only (x_p is 0 when rank==0).
				matrix_core::MatrixMutView out{};
				if (info.rank == n) {
						out = xp;
				} else if (n_basis.cols >= 6) {
						out = n_basis; // fully-free case (e.g. A=0)
				} else {
						const std::uint8_t out_cols = static_cast<std::uint8_t>(n_basis.cols + 1u);
						const matrix_core::ErrorCode ec_o = matrix_core::matrix_alloc(persist_, n, out_cols, &out);
						if (!matrix_core::is_ok(ec_o)) {
								show_message("common.out_of_memory"_tx);
								return;
						}
						for (std::uint8_t r = 0; r < n; r++) {
								out.at_mut(r, 0) = xp.at(r, 0);
								for (std::uint8_t c = 0; c < n_basis.cols; c++)
										out.at_mut(r, static_cast<std::uint8_t>(c + 1u)) = n_basis.at(r, c);
						}
				}

				tx.commit();
				expl_ = std::move(expl);
				REQUIRE(pop(), "pop failed (solve rref)");
				Page p = Page::make_result_matrix(s.op, a_slot, b_slot, expl_.available(), out.rows, out.cols, out.stride, out.data);
				p.u.result.i = info.rank;
				p.u.result.j = nullity;
				p.u.result.num = piv_mask;
				p.u.result.den = 1;
				REQUIRE(push(p), "push solve rref result failed");
				return;
		}

#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
		if (s.op == OperationId::Projection) {
				matrix_core::ArenaScope tx(persist_);
				matrix_core::MatrixMutView out_proj{};
				matrix_core::MatrixMutView out_orth{};

				const matrix_core::ErrorCode ec1 = matrix_core::matrix_alloc(persist_, a.rows, a.cols, &out_proj);
				const matrix_core::ErrorCode ec2 = matrix_core::matrix_alloc(persist_, a.rows, a.cols, &out_orth);
				if (!matrix_core::is_ok(ec1) || !matrix_core::is_ok(ec2)) {
						SHELL_DBG("[op] proj alloc failed ec1=%u ec2=%u used=%u/%u\n",
						        (unsigned)ec1,
						        (unsigned)ec2,
						        (unsigned)persist_.used(),
						        (unsigned)persist_.capacity());
						show_message("common.out_of_memory"_tx);
						return;
				}

				matrix_core::ExplainOptions opts_steps;
				opts_steps.enable = true;
				opts_steps.persist = &persist_;

				matrix_core::Explanation expl;
				matrix_core::ProjDecomposeResult res{};
				const matrix_core::Error err = matrix_core::op_proj_decompose_u_onto_v(a, b, out_proj, out_orth, &res, &expl, opts_steps);

				SHELL_DBG("[op] proj err=%u a=%ux%u b=%ux%u k=",
				        (unsigned)err.code,
				        (unsigned)err.a.rows,
				        (unsigned)err.a.cols,
				        (unsigned)err.b.rows,
				        (unsigned)err.b.cols);
				dbg_print_rational(res.k);
				SHELL_DBG("\n");

				if (!matrix_core::is_ok(err)) {
						if (err.code == matrix_core::ErrorCode::InvalidDimension)
								show_message("msg.need_vectors"_tx);
						else if (err.code == matrix_core::ErrorCode::DimensionMismatch) {
								fmt_buf_[0] = '\0';
								matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
								w.append("msg.need_same_length_prefix"_tx);
								w.put(static_cast<char>('A' + a_slot));
								w.append("=");
								w.append_u64(a.rows);
								w.put('x');
								w.append_u64(a.cols);
								w.append(", ");
								w.put(static_cast<char>('A' + b_slot));
								w.append("=");
								w.append_u64(b.rows);
								w.put('x');
								w.append_u64(b.cols);
								show_message(fmt_buf_);
						} else if (err.code == matrix_core::ErrorCode::DivisionByZero) {
								show_message("msg.v_zero_vector"_tx);
						} else {
								if (err.code == matrix_core::ErrorCode::Internal)
										fail_fast("update_slot_pick: projection returned Internal");
								show_message("common.error"_tx);
						}
						return;
				}

				tx.commit();
				expl_ = std::move(expl);
				persist_tail_mark_ = persist_.mark();
				REQUIRE(pop(), "pop failed (projection)");
				REQUIRE(push(Page::make_projection_result(a_slot,
				                b_slot,
				                expl_.available(),
				                out_proj.rows,
				                out_proj.cols,
				                out_proj.stride,
				                out_proj.data,
				                out_orth.stride,
				                out_orth.data,
				                res.k.num(),
				                res.k.den())),
				        "push projection result failed");
				return;
		}
#endif

#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
		if (s.op == OperationId::Cramer) {
				if (a.rows != a.cols) {
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append("msg.a_must_be_square_prefix"_tx);
						w.put(static_cast<char>('A' + a_slot));
						w.append("=");
						w.append_u64(a.rows);
						w.put('x');
						w.append_u64(a.cols);
						show_message(fmt_buf_);
						return;
				}
				if (b.rows != a.rows || b.cols != 1) {
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append("msg.need_b_nx1_prefix"_tx);
						w.append_u64(a.rows);
						w.put('x');
						w.append_u64(a.cols);
						w.append("msg.b_is_prefix"_tx);
						w.append_u64(b.rows);
						w.put('x');
						w.append_u64(b.cols);
						show_message(fmt_buf_);
						return;
				}

				matrix_core::ArenaScope tx(persist_);
				matrix_core::MatrixMutView x_out{};
				const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, a.rows, 1, &x_out);
				if (!matrix_core::is_ok(ec)) {
						SHELL_DBG("[op] cramer alloc failed ec=%u used=%u/%u\n",
						        (unsigned)ec,
						        (unsigned)persist_.used(),
						        (unsigned)persist_.capacity());
						show_message("common.out_of_memory"_tx);
						return;
				}

				matrix_core::ArenaScratchScope scratch_tx(scratch_);
				const matrix_core::Error err = matrix_core::op_cramer_solve(a, b, scratch_, x_out);
				SHELL_DBG("[op] cramer err=%u a=%ux%u b=%ux%u\n",
				        (unsigned)err.code,
				        (unsigned)err.a.rows,
				        (unsigned)err.a.cols,
				        (unsigned)err.b.rows,
				        (unsigned)err.b.cols);
				if (!matrix_core::is_ok(err)) {
						if (err.code == matrix_core::ErrorCode::Singular)
								show_message("msg.no_unique_solution"_tx);
						else if (err.code == matrix_core::ErrorCode::NotSquare || err.code == matrix_core::ErrorCode::DimensionMismatch ||
						         err.code == matrix_core::ErrorCode::Internal)
								fail_fast("update_slot_pick: cramer returned unexpected error");
						else
								show_message("common.error"_tx);
						return;
				}

				tx.commit();
				expl_ = {};
				persist_tail_mark_ = persist_.mark();
				REQUIRE(pop(), "pop failed (cramer)");
				REQUIRE(push(Page::make_result_matrix(
				                OperationId::Cramer, a_slot, b_slot, true, x_out.rows, x_out.cols, x_out.stride, x_out.data)),
				        "push cramer result failed");
				return;
		}
#endif

		matrix_core::Dim out_dim{};
		if (s.op == OperationId::Add || s.op == OperationId::Sub)
				out_dim = a.dim();
		else
				out_dim = {a.rows, b.cols};

		SHELL_DBG("[op] %s dims A=%ux%u B=%ux%u out=%ux%u\n",
		        op_name(s.op),
		        (unsigned)a.rows,
		        (unsigned)a.cols,
		        (unsigned)b.rows,
		        (unsigned)b.cols,
		        (unsigned)out_dim.rows,
		        (unsigned)out_dim.cols);

		matrix_core::ArenaScope tx(persist_);
		matrix_core::MatrixMutView out{};
		const matrix_core::ErrorCode ec = matrix_core::matrix_alloc(persist_, out_dim.rows, out_dim.cols, &out);
		if (!matrix_core::is_ok(ec)) {
				SHELL_DBG("[op] alloc failed ec=%u used=%u/%u\n", (unsigned)ec, (unsigned)persist_.used(), (unsigned)persist_.capacity());
				show_message("common.out_of_memory"_tx);
				return;
		}
		SHELL_DBG("[op] alloc ok out.data=%p stride=%u used=%u/%u\n",
		        (void*)out.data,
		        (unsigned)out.stride,
		        (unsigned)persist_.used(),
		        (unsigned)persist_.capacity());

		matrix_core::Explanation expl;
		matrix_core::ExplainOptions opts_steps;
		opts_steps.enable = true;
		opts_steps.persist = &persist_;
		matrix_core::Error err{};
		if (s.op == OperationId::Add)
				err = matrix_core::op_add(a, b, out, &expl, opts_steps);
		else if (s.op == OperationId::Sub)
				err = matrix_core::op_sub(a, b, out, &expl, opts_steps);
		else
				err = matrix_core::op_mul(a, b, out, &expl, opts_steps);

		SHELL_DBG("[op] %s err=%u a=%ux%u b=%ux%u\n",
		        op_name(s.op),
		        (unsigned)err.code,
		        (unsigned)err.a.rows,
		        (unsigned)err.a.cols,
		        (unsigned)err.b.rows,
		        (unsigned)err.b.cols);
		if (!matrix_core::is_ok(err)) {
				if (err.code == matrix_core::ErrorCode::DimensionMismatch) {
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						if (s.op == OperationId::Mul)
								w.append("msg.inner_sizes_match_prefix"_tx);
						else
								w.append("msg.need_same_size_prefix"_tx);
						w.put(static_cast<char>('A' + a_slot));
						w.append("=");
						w.append_u64(a.rows);
						w.put('x');
						w.append_u64(a.cols);
						w.append(", ");
						w.put(static_cast<char>('A' + b_slot));
						w.append("=");
						w.append_u64(b.rows);
						w.put('x');
						w.append_u64(b.cols);
						show_message(fmt_buf_);
				} else if (err.code == matrix_core::ErrorCode::Internal || err.code == matrix_core::ErrorCode::InvalidDimension)
						fail_fast("update_slot_pick: binary op returned unexpected error");
				else
						show_message("common.error"_tx);
				return;
		}

		tx.commit();
		expl_ = std::move(expl);
		persist_tail_mark_ = persist_.mark();
		REQUIRE(pop(), "pop failed (binary op)");
		REQUIRE(push(Page::make_result_matrix(s.op, a_slot, b_slot, expl_.available(), out.rows, out.cols, out.stride, out.data)),
		        "push binary result failed");
}

} // namespace matrix_shell
