#include "matrix_core/ops.hpp"

#include "matrix_core/det_detail.hpp"
#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/writer.hpp"

namespace matrix_core {
ErrorCode Explanation::render_det(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView work;
		ErrorCode ec = matrix_work_view(out.work, input.rows, input.cols, &work);
		if (!is_ok(ec))
				return ec;
		if (index == 0)
				return latex::write_matrix_display(input, latex::MatrixBrackets::VMatrix, {out.latex, out.latex_cap});
		if (index == op_count + 1) {
				Writer w{out.latex, out.latex_cap};
				w.append("$$\\det(A) = ");
				w.append_rational_latex(value);
				w.append("$$");
				return w.status();
		}
		ec = matrix_copy(input, work);
		if (!is_ok(ec))
				return ec;
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

Error op_det(MatrixView a, MatrixMutView storage, Rational* out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		if (!out)
				return {ErrorCode::Internal};
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		MatrixMutView work;
		ErrorCode ec = matrix_work_view(storage, a.rows, a.cols, &work);
		if (!is_ok(ec))
				return {ec};
		ec = matrix_copy(a, work);
		if (!is_ok(ec))
				return {ec};
		Rational det;
		std::size_t op_count = 0;
		ec = detail::det_elim(work, &det, &op_count, static_cast<std::size_t>(-1), nullptr);
		if (!is_ok(ec))
				return {ec, a.dim()};
		*out = det;
		if (expl) {
				expl->kind = Explanation::Kind::Det;
				expl->input = a;
				expl->value = det;
				expl->op_count = op_count;
		}
		return {};
}
} // namespace matrix_core
