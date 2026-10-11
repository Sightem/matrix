#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/writer.hpp"

namespace matrix_core {
ErrorCode Explanation::render_binary(std::size_t index, const StepRenderBuffers& out) const noexcept {
		if (index == 0) {
				Writer w{out.latex, out.latex_cap};
				w.append("$$C = A");
				w.append(kind == Kind::Add ? " + " : kind == Kind::Sub ? " - " : " \\cdot ");
				w.append("B$$");
				return w.status();
		}
		return latex::write_matrix_display(result, latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

ErrorCode Explanation::render_transpose(std::size_t index, const StepRenderBuffers& out) const noexcept {
		return latex::write_matrix_display(index == 0 ? input : result, latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

Error op_add(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		ErrorCode ec = matrix_add(a, b, out);
		if (!is_ok(ec))
				return {ec, a.dim(), b.dim()};
		if (expl) {
				expl->kind = Explanation::Kind::Add;
				expl->result = out.view();
		}
		return {};
}

Error op_sub(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		ErrorCode ec = matrix_sub(a, b, out);
		if (!is_ok(ec))
				return {ec, a.dim(), b.dim()};
		if (expl) {
				expl->kind = Explanation::Kind::Sub;
				expl->result = out.view();
		}
		return {};
}

Error op_mul(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		ErrorCode ec = matrix_mul(a, b, out);
		if (!is_ok(ec))
				return {ec, a.dim(), b.dim()};
		if (expl) {
				expl->kind = Explanation::Kind::Mul;
				expl->result = out.view();
		}
		return {};
}

Error op_transpose(MatrixView a, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		ErrorCode ec = matrix_transpose(a, out);
		if (!is_ok(ec))
				return {ec, a.dim()};
		if (expl) {
				expl->kind = Explanation::Kind::Transpose;
				expl->input = a;
				expl->result = out.view();
		}
		return {};
}
} // namespace matrix_core
