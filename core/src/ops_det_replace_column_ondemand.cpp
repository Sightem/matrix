#include "matrix_core/ops.hpp"

#if MATRIX_CORE_ENABLE_CRAMER
#include "matrix_core/det_detail.hpp"
#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/writer.hpp"

namespace matrix_core {
namespace {
ErrorCode build_matrix(MatrixView a, MatrixView b, std::uint8_t col, MatrixMutView storage, MatrixMutView* out) noexcept {
		MatrixMutView work;
		ErrorCode ec = matrix_work_view(storage, a.rows, a.cols, &work);
		if (!is_ok(ec))
				return ec;
		ec = matrix_copy(a, work);
		if (!is_ok(ec))
				return ec;
		for (std::uint8_t row = 0; row < a.rows; row++)
				work.at_mut(row, col) = b.at(row, 0);
		*out = work;
		return ErrorCode::Ok;
}
} // namespace

ErrorCode Explanation::render_det_replace(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView work;
		ErrorCode ec = build_matrix(input, other, col, out.work, &work);
		if (!is_ok(ec))
				return ec;
		if (index == 0)
				return latex::write_matrix_display(work.view(), latex::MatrixBrackets::VMatrix, {out.latex, out.latex_cap});
		if (index == op_count + 1) {
				Writer w{out.latex, out.latex_cap};
				w.append("$$\\det(A_{");
				w.append_index1(col);
				w.append("}) = ");
				w.append_rational_latex(value);
				w.append("$$");
				return w.status();
		}
		Rational ignored;
		RowOp last{};
		std::size_t ops = 0;
		ec = detail::det_elim(work, &ignored, &ops, index, &last);
		if (!is_ok(ec))
				return ec;
		if (ops < index)
				return ErrorCode::StepOutOfRange;
		if (out.caption) {
				ec = row_op_caption(last, out.caption, out.caption_cap);
				if (!is_ok(ec))
						return ec;
		}
		return latex::write_matrix_display(work.view(), latex::MatrixBrackets::VMatrix, {out.latex, out.latex_cap});
}

Error op_det_replace_column(MatrixView a, MatrixView b, std::uint8_t col, MatrixMutView storage,
        Rational* out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		if (!out)
				return {ErrorCode::Internal};
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		if (b.rows != a.rows || b.cols != 1)
				return err_dim_mismatch(a.dim(), b.dim());
		if (col >= a.cols) {
				Error err{ErrorCode::IndexOutOfRange, a.dim()};
				err.i = col;
				return err;
		}
		if (!b.data)
				return {ErrorCode::Internal};
		if (b.stride < b.cols)
				return err_invalid_dim(b.dim());
		MatrixMutView work;
		ErrorCode ec = build_matrix(a, b, col, storage, &work);
		if (!is_ok(ec))
				return {ec};
		Rational det;
		std::size_t op_count = 0;
		ec = detail::det_elim(work, &det, &op_count, static_cast<std::size_t>(-1), nullptr);
		if (!is_ok(ec))
				return {ec, a.dim()};
		*out = det;
		if (expl) {
				expl->kind = Explanation::Kind::DetReplace;
				expl->input = a;
				expl->other = b;
				expl->col = col;
				expl->value = det;
				expl->op_count = op_count;
		}
		return {};
}
} // namespace matrix_core
#endif
