#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"
#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/spaces.hpp"
#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::kKbGroup6;
using detail::op_is_binary;
}

void App::render_slot_pick(const SlotPickState& s) noexcept {
		render_header(nullptr);
		gfx_PrintString(detail::op_name(s.op));
		gfx_PrintString(": ");
		gfx_PrintString(tr(op_is_binary(s.op) ? (s.stage == 0 ? TextId::SlotPickPick1 : TextId::SlotPickPick2) : TextId::SlotPickPick));
		render_footer_hint(tr(TextId::FooterSelectBack));
		for (std::uint8_t row = 0; row < ui::kListVisible && s.scroll + row < kSlotCount; row++) {
				const std::uint8_t idx = static_cast<std::uint8_t>(s.scroll + row);
				ui::list_row(row, idx == s.cursor);
				detail::render_slot(idx, slots_[idx]);
		}
}

void App::update_slot_pick(SlotPickState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				if (op_is_binary(s.op) && s.stage == 1)
						s.stage = 0;
				else
						pop();
				return;
		}
		detail::update_list(input_, s.cursor, s.scroll, kSlotCount);
		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;
		const std::uint8_t sel = s.cursor;
		if (!slots_[sel].is_set()) {
				show_message(tr(TextId::CommonSlotUnset));
				return;
		}
		if (op_is_binary(s.op) && s.stage == 0) {
				s.slot_a = sel;
				s.stage = 1;
				return;
		}
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
		if (s.op == OperationId::CofactorElement) {
				const auto a = slots_[sel].view_active();
				if (operation_ok(s.op, a.rows == a.cols ? matrix_core::ErrorCode::Ok : matrix_core::ErrorCode::NotSquare, sel))
						push(Page::make_cofactor_element(sel, a.rows));
				return;
		}
#endif
		execute_operation(s.op, op_is_binary(s.op) ? s.slot_a : sel, op_is_binary(s.op) ? sel : 0);
}

bool App::operation_ok(OperationId op, matrix_core::ErrorCode error, std::uint8_t slot_a, std::uint8_t slot_b) noexcept {
		using matrix_core::ErrorCode;
		if (error == ErrorCode::Ok)
				return true;
		const auto a = slots_[slot_a].view_active();
		const auto b = slots_[slot_b].view_active();
		TextId message = TextId::CommonError;
		TextId prefix = TextId::None;
		bool both = false;
		switch (error) {
		case ErrorCode::NotSquare:
				prefix = op == OperationId::Det ? TextId::MessageDetRequiresSquarePrefix :
				        op == OperationId::Inverse ? TextId::MessageInverseRequiresSquarePrefix :
				        op == OperationId::Cramer ? TextId::MessageAMustBeSquarePrefix : TextId::MessageRequiresSquarePrefix;
				break;
		case ErrorCode::DimensionMismatch:
				if (op == OperationId::SolveRref) {
						message = TextId::MessageNeedBmx1;
				} else if (op == OperationId::Cramer) {
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append(tr(TextId::MessageNeedBNx1Prefix));
						w.append_u64(a.rows);
						w.put('x');
						w.append_u64(a.cols);
						w.append(tr(TextId::MessageBisPrefix));
						w.append_u64(b.rows);
						w.put('x');
						w.append_u64(b.cols);
						show_message(fmt_buf_);
						return false;
				} else if (op == OperationId::Dot || op == OperationId::Projection) {
						prefix = TextId::MessageNeedSameLengthPrefix;
						both = true;
				} else if (op == OperationId::Add || op == OperationId::Sub || op == OperationId::Mul) {
						prefix = op == OperationId::Mul ? TextId::MessageInnerSizesMustMatchPrefix : TextId::MessageNeedSameSizePrefix;
						both = true;
				} else {
						detail::fail_fast("Unexpected operation output dimensions");
				}
				break;
		case ErrorCode::InvalidDimension:
				if (op == OperationId::Dot || op == OperationId::Projection)
						message = TextId::MessageNeedVectors;
				else if (op == OperationId::Cross)
						message = TextId::MessageNeed3DVectors;
				else if (op == OperationId::MinorMatrix && a.rows <= 1)
						message = TextId::MessageNeedNGe2;
				else
						detail::fail_fast("Unexpected operation input dimensions");
				break;
		case ErrorCode::Singular:
				message = op == OperationId::Inverse ? TextId::MessageSingularNoInverse : TextId::MessageNoUniqueSolution;
				break;
		case ErrorCode::NoSolution:
				message = TextId::MessageNoSolution;
				break;
		case ErrorCode::DivisionByZero:
				if (op == OperationId::Projection)
						message = TextId::MessageVZeroVector;
				break;
		case ErrorCode::IndexOutOfRange:
		case ErrorCode::Internal:
				detail::fail_fast("Unexpected operation error");
		default:
				break;
		}
		if (prefix != TextId::None) {
				matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
				w.append(tr(prefix));
				auto shape = [&](std::uint8_t slot, matrix_core::MatrixView matrix) noexcept {
						w.put(static_cast<char>('A' + slot));
						w.put('=');
						w.append_u64(matrix.rows);
						w.put('x');
						w.append_u64(matrix.cols);
				};
				shape(slot_a, a);
				if (both) {
						w.append(", ");
						shape(slot_b, b);
				}
				show_message(fmt_buf_);
		} else {
				show_message(tr(message));
		}
		return false;
}

void App::execute_operation(OperationId op, std::uint8_t slot_a, std::uint8_t slot_b,
        [[maybe_unused]] std::uint8_t row, [[maybe_unused]] std::uint8_t col) noexcept {
		using namespace matrix_core;
		const MatrixView a = slots_[slot_a].view_active();
		const MatrixView b = slots_[slot_b].view_active();
		MatrixMutView out{a.rows, a.cols, a.cols, calculation_.result};
		if (op == OperationId::Transpose)
				out = {a.cols, a.rows, a.rows, calculation_.result};
		else if (op == OperationId::Mul)
				out = {a.rows, b.cols, b.cols, calculation_.result};
		else if (op == OperationId::Cramer)
				out = {a.rows, 1, 1, calculation_.result};
		ResultState result{op, slot_a, slot_b, false, {.matrix = out.view()}};
		calculation_.explanation = {};
		Explanation* explanation = &calculation_.explanation;
		ErrorCode error = ErrorCode::Ok;
		switch (op) {
		case OperationId::Add:
				error = op_add(a, b, out, explanation).code;
				break;
		case OperationId::Sub:
				error = op_sub(a, b, out, explanation).code;
				break;
		case OperationId::Mul:
				error = op_mul(a, b, out, explanation).code;
				break;
		case OperationId::Transpose:
				error = op_transpose(a, out, explanation).code;
				break;
		case OperationId::Inverse:
				error = op_inverse(a, work(), out, explanation).code;
				break;
		case OperationId::Det:
				result.value = {.scalar = Rational::from_int(0)};
				error = op_det(a, work(), &result.value.scalar, explanation).code;
				break;
		case OperationId::Dot:
				result.value = {.scalar = Rational::from_int(0)};
				error = op_dot(a, b, &result.value.scalar, explanation).code;
				break;
		case OperationId::Cross:
				error = op_cross(a, b, out, explanation).code;
				break;
		case OperationId::Ref:
		case OperationId::Rref:
				error = op_echelon(a, op == OperationId::Ref ? EchelonKind::Ref : EchelonKind::Rref, out, explanation).code;
				break;
		case OperationId::SolveRref: {
				SolveResult solution{};
				error = op_solve_rref(a, b, {6, 7, 7, calculation_.input}, work(),
				        {6, 1, 1, calculation_.result}, {6, 6, 6, calculation_.result + 6}, &solution, explanation).code;
				if (is_ok(error)) {
						const auto& info = solution.info;
						result.value = {.solution = {solution.particular, solution.null_basis,
						        {info.rank, info.nullity, info.pivot_mask}, false}};
				}
				break;
		}
		case OperationId::SpanTest:
		case OperationId::IndepTest:
		case OperationId::ColSpaceBasis:
		case OperationId::RowSpaceBasis:
		case OperationId::NullSpaceBasis:
		case OperationId::LeftNullSpaceBasis: {
				MatrixView input = a;
				if (op == OperationId::LeftNullSpaceBasis) {
						MatrixMutView transposed{a.cols, a.rows, a.rows, calculation_.input};
						error = matrix_transpose(a, transposed);
						input = transposed.view();
				}
				MatrixMutView rref{input.rows, input.cols, 12, work_};
				SpaceInfo info{};
				if (is_ok(error))
						error = op_echelon(input, EchelonKind::Rref, rref, explanation).code;
				if (is_ok(error))
						error = space_info_from_rref(rref.view(), input.cols, &info);
				if (!is_ok(error))
						break;
				const SpaceSummary summary{info.rank, info.nullity, info.pivot_mask};
				if (op == OperationId::SpanTest || op == OperationId::IndepTest) {
						result.value = {.predicate = {info.rank == (op == OperationId::SpanTest ? a.rows : a.cols), a.dim(), summary}};
				} else {
						const MatrixMutView storage{6, 6, 6, calculation_.result};
						if (op == OperationId::ColSpaceBasis)
								error = space_col_basis(a, info, storage, &out);
						else if (op == OperationId::RowSpaceBasis)
								error = space_row_basis(rref.view(), input.cols, info, storage, &out);
						else
								error = space_null_basis(rref.view(), input.cols, info, storage, &out);
						result.value = {.basis = {out.view(), summary}};
				}
				break;
		}
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
		case OperationId::Projection: {
				MatrixMutView orthogonal{a.rows, a.cols, a.cols, calculation_.result + 6};
				ProjDecomposeResult projection{};
				error = op_proj_decompose_u_onto_v(a, b, out, orthogonal, &projection, explanation).code;
				result.value = {.projection = {out.view(), orthogonal.view(), projection.k, false}};
				break;
		}
#endif
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
		case OperationId::Cramer:
				error = op_cramer_solve(a, b, work(), out).code;
				break;
#endif
#if MATRIX_SHELL_ENABLE_MINOR_MATRIX && MATRIX_CORE_ENABLE_MINOR_MATRIX
		case OperationId::MinorMatrix:
				error = op_minor_matrix(a, work(), out).code;
				break;
#endif
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
		case OperationId::CofactorElement:
				result.value = {.cofactor = {Rational::from_int(0), row, col}};
				error = op_cofactor_element(a, row, col, work(), &result.value.cofactor.value, explanation).code;
				break;
#endif
		default:
				detail::fail_fast("Unhandled OperationId in execute_operation");
		}
		if (!operation_ok(op, error, slot_a, slot_b))
				return;
		result.has_steps = op == OperationId::Cramer || explanation->available();
		if (op == OperationId::CofactorElement)
				pop();
		top() = {PageKind::Result, {.result = result}};
}

}
