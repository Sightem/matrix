#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/row_reduction.hpp"

namespace matrix_core {
ErrorCode Explanation::render_echelon(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView work;
		ErrorCode ec = matrix_work_view(out.work, input.rows, input.cols, &work);
		if (!is_ok(ec))
				return ec;
		if (index == 0)
				return latex::write_matrix_display(input, latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
		ec = matrix_copy(input, work);
		if (!is_ok(ec))
				return ec;
		OpObserver obs;
		obs.target = index;
		ec = echelon_apply(work, kind == Kind::Ref ? EchelonKind::Ref : EchelonKind::Rref, &obs);
		if (!is_ok(ec))
				return ec;
		if (obs.count < index)
				return ErrorCode::StepOutOfRange;
		if (out.caption) {
				ec = row_op_caption(obs.last_op, out.caption, out.caption_cap);
				if (!is_ok(ec))
						return ec;
		}
		return latex::write_matrix_display(work.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

Error op_echelon(MatrixView a, EchelonKind kind, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		if (out.rows != a.rows || out.cols != a.cols)
				return {ErrorCode::DimensionMismatch, a.dim(), out.dim()};
		ErrorCode ec = matrix_copy(a, out);
		if (!is_ok(ec))
				return {ec};
		OpObserver obs;
		ec = echelon_apply(out, kind, &obs);
		if (!is_ok(ec))
				return {ec, a.dim()};
		if (expl) {
				expl->kind = kind == EchelonKind::Ref ? Explanation::Kind::Ref : Explanation::Kind::Rref;
				expl->input = a;
				expl->op_count = obs.count;
		}
		return {};
}
} // namespace matrix_core
