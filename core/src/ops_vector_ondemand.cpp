#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/writer.hpp"


namespace matrix_core {
namespace {
struct VecInfo {
		std::uint8_t len = 0;
		bool is_row = false;
};

ErrorCode vec_info(In const MatrixView& m, Out VecInfo* out) noexcept {
		if (!m.data)
				return ErrorCode::Internal;
		if (m.stride < m.cols)
				return ErrorCode::InvalidDimension;

		if (m.rows == 1 && m.cols >= 1) {
				out->len = m.cols;
				out->is_row = true;
				return ErrorCode::Ok;
		}
		if (m.cols == 1 && m.rows >= 1) {
				out->len = m.rows;
				out->is_row = false;
				return ErrorCode::Ok;
		}
		return ErrorCode::InvalidDimension;
}

const Rational& vec_at(In const MatrixView& m, In const VecInfo& vi, In std::uint8_t i) noexcept {
		return vi.is_row ? m.at(0, i) : m.at(i, 0);
}

Rational& vec_at_mut(InOut MatrixMutView& m, In const VecInfo& vi, In std::uint8_t i) noexcept {
		return vi.is_row ? m.at_mut(0, i) : m.at_mut(i, 0);
}

ErrorCode dot_raw(In const MatrixView& u, In const VecInfo& ui, In const MatrixView& v, In const VecInfo& vi, Out Rational* out) noexcept {
		Rational sum = Rational::from_int(0);
		for (std::uint8_t i = 0; i < ui.len; i++) {
				Rational prod;
				ErrorCode ec = rational_mul(vec_at(u, ui, i), vec_at(v, vi, i), &prod);
				if (!is_ok(ec))
						return ec;
				Rational next;
				ec = rational_add(sum, prod, &next);
				if (!is_ok(ec))
						return ec;
				sum = next;
		}

		*out = sum;
		return ErrorCode::Ok;
}

ErrorCode dot_of(In const MatrixView& u, In const MatrixView& v, Out Rational* out, Out VecInfo* ui_out, Out VecInfo* vi_out) noexcept {
		VecInfo ui;
		ErrorCode ec = vec_info(u, &ui);
		if (!is_ok(ec))
				return ec;
		VecInfo vi;
		ec = vec_info(v, &vi);
		if (!is_ok(ec))
				return ec;
		if (ui.len != vi.len)
				return ErrorCode::DimensionMismatch;
		if (ui.len > kMaxRows)
				return ErrorCode::InvalidDimension;

		*ui_out = ui;
		*vi_out = vi;
		return dot_raw(u, ui, v, vi, out);
}

ErrorCode cross_raw(In const MatrixView& u,
        In const VecInfo& ui,
        In const MatrixView& v,
        In const VecInfo& vi,
        Out MatrixMutView& out,
        In const VecInfo& oi) noexcept {
		// out = u x v
		Rational prod1;
		Rational prod2;
		Rational comp;

		// c1 = u2*v3 - u3*v2
		ErrorCode ec = rational_mul(vec_at(u, ui, 1), vec_at(v, vi, 2), &prod1);
		if (!is_ok(ec))
				return ec;
		ec = rational_mul(vec_at(u, ui, 2), vec_at(v, vi, 1), &prod2);
		if (!is_ok(ec))
				return ec;
		ec = rational_sub(prod1, prod2, &comp);
		if (!is_ok(ec))
				return ec;
		vec_at_mut(out, oi, 0) = comp;

		// c2 = u3*v1 - u1*v3
		ec = rational_mul(vec_at(u, ui, 2), vec_at(v, vi, 0), &prod1);
		if (!is_ok(ec))
				return ec;
		ec = rational_mul(vec_at(u, ui, 0), vec_at(v, vi, 2), &prod2);
		if (!is_ok(ec))
				return ec;
		ec = rational_sub(prod1, prod2, &comp);
		if (!is_ok(ec))
				return ec;
		vec_at_mut(out, oi, 1) = comp;

		// c3 = u1*v2 - u2*v1
		ec = rational_mul(vec_at(u, ui, 0), vec_at(v, vi, 1), &prod1);
		if (!is_ok(ec))
				return ec;
		ec = rational_mul(vec_at(u, ui, 1), vec_at(v, vi, 0), &prod2);
		if (!is_ok(ec))
				return ec;
		ec = rational_sub(prod1, prod2, &comp);
		if (!is_ok(ec))
				return ec;
		vec_at_mut(out, oi, 2) = comp;
		return ErrorCode::Ok;
}

#if MATRIX_CORE_ENABLE_PROJECTION
ErrorCode proj_decompose_raw_impl(In const MatrixView& u,
        In const VecInfo& ui,
        In const MatrixView& v,
        In const VecInfo& vi,
        Out MatrixMutView& out_proj,
        Out MatrixMutView& out_orth,
        Out ProjDecomposeResult& out) noexcept {
		ErrorCode ec = dot_raw(u, ui, v, vi, &out.dot_uv);
		if (!is_ok(ec))
				return ec;
		ec = dot_raw(v, vi, v, vi, &out.dot_vv);
		if (!is_ok(ec))
				return ec;
		ec = rational_div(out.dot_uv, out.dot_vv, &out.k);
		if (!is_ok(ec))
				return ec;

		// proj = k * v
		for (std::uint8_t i = 0; i < ui.len; i++) {
				Rational term;
				ec = rational_mul(out.k, vec_at(v, vi, i), &term);
				if (!is_ok(ec))
						return ec;
				vec_at_mut(out_proj, ui, i) = term;
		}

		// orth = u - proj
		for (std::uint8_t i = 0; i < ui.len; i++) {
				Rational term;
				ec = rational_sub(vec_at(u, ui, i), vec_at(out_proj.view(), ui, i), &term);
				if (!is_ok(ec))
						return ec;
				vec_at_mut(out_orth, ui, i) = term;
		}

		// ||proj||^2 and ||orth||^2
		ec = dot_raw(out_proj.view(), ui, out_proj.view(), ui, &out.proj_norm2);
		if (!is_ok(ec))
				return ec;
		return dot_raw(out_orth.view(), ui, out_orth.view(), ui, &out.orth_norm2);
}


#endif // MATRIX_CORE_ENABLE_PROJECTION

} // namespace

ErrorCode Explanation::render_dot(std::size_t, const StepRenderBuffers& out) const noexcept {
		const VecInfo ui{input.rows == 1 ? input.cols : input.rows, input.rows == 1};
		const VecInfo vi{other.rows == 1 ? other.cols : other.rows, other.rows == 1};
		Writer w{out.latex, out.latex_cap};
		w.append("$$u\\cdot v = ");
		for (std::uint8_t i = 0; i < ui.len; i++) {
				if (i != 0)
						w.append(" + ");
				w.put('(');
				w.append_rational_latex(vec_at(input, ui, i));
				w.append(")\\cdot(");
				w.append_rational_latex(vec_at(other, vi, i));
				w.put(')');
		}
		w.append(" = ");
		w.append_rational_latex(value);
		w.append("$$");
		return w.status();
}

ErrorCode Explanation::render_cross(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView tmp;
		ErrorCode ec = matrix_work_view(out.work, result.rows, result.cols, &tmp);
		if (!is_ok(ec))
				return ec;
		if (index == 0) {
				Writer w{out.latex, out.latex_cap};
				w.append("$$u\\times v = \\begin{bmatrix}u_2 v_3 - u_3 v_2 \\\\ u_3 v_1 - u_1 v_3 \\\\ u_1 v_2 - u_2 v_1\\end{bmatrix}$$");
				return w.status();
		}
		const VecInfo ui{3, input.rows == 1};
		const VecInfo vi{3, other.rows == 1};
		ec = cross_raw(input, ui, other, vi, tmp, ui);
		if (!is_ok(ec))
				return ec;
		if (out.caption && out.caption_cap) {
				Writer w{out.caption, out.caption_cap};
				w.append("$u\\times v$");
				if (!is_ok(w.status()))
						return w.status();
		}
		return latex::write_matrix_display(tmp.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

#if MATRIX_CORE_ENABLE_PROJECTION
ErrorCode Explanation::render_proj(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView work;
		ErrorCode ec = matrix_work_view(out.work, result.rows, result.cols, &work);
		if (!is_ok(ec))
				return ec;
		if (index != 4 && index != 5) {
				Writer w{out.latex, out.latex_cap};
				switch (index) {
				case 0:
						w.append("$$proj_v(u) = \\frac{u\\cdot v}{v\\cdot v} v$$");
						return w.status();
				case 1:
						w.append("$$u\\cdot v = ");
						w.append_rational_latex(value);
						break;
				case 2:
						w.append("$$v\\cdot v = ");
						w.append_rational_latex(dot_vv);
						break;
				case 3:
						w.append("$$k = \\frac{u\\cdot v}{v\\cdot v} = ");
						w.append_rational_latex(k);
						break;
				case 6:
						w.append("$$||proj||^2 = ");
						w.append_rational_latex(proj_norm2);
						break;
				case 7:
						w.append("$$||orth||^2 = ");
						w.append_rational_latex(orth_norm2);
						break;
				default:
						return ErrorCode::StepOutOfRange;
				}
				w.append("$$");
				return w.status();
		}
		const VecInfo ui{input.rows == 1 ? input.cols : input.rows, input.rows == 1};
		const VecInfo vi{other.rows == 1 ? other.cols : other.rows, other.rows == 1};
		for (std::uint8_t i = 0; i < ui.len; i++) {
				Rational term;
				ec = rational_mul(k, vec_at(other, vi, i), &term);
				if (!is_ok(ec))
						return ec;
				if (index == 5) {
						Rational orth;
						ec = rational_sub(vec_at(input, ui, i), term, &orth);
						if (!is_ok(ec))
								return ec;
						term = orth;
				}
				vec_at_mut(work, ui, i) = term;
		}
		if (out.caption && out.caption_cap) {
				Writer w{out.caption, out.caption_cap};
				w.append(index == 4 ? "$proj = kv$" : "$orth = u - proj$");
				if (!is_ok(w.status()))
						return w.status();
		}
		return latex::write_matrix_display(work.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}
#endif

Error op_dot(In MatrixView u, In MatrixView v, Out Rational* out, Out Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		Error err;
		if (!out)
				return {ErrorCode::Internal};

		Rational dot;
		VecInfo ui;
		VecInfo vi;
		ErrorCode ec = dot_of(u, v, &dot, &ui, &vi);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = u.dim();
				err.b = v.dim();
				return err;
		}

		*out = dot;

		if (expl) {
				expl->kind = Explanation::Kind::Dot;
				expl->input = u;
				expl->other = v;
				expl->value = dot;
		}

		return err;
}

Error op_cross(In MatrixView u, In MatrixView v, Out MatrixMutView out, Out Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		Error err;
		VecInfo ui;
		ErrorCode ec = vec_info(u, &ui);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = u.dim();
				return err;
		}
		VecInfo vi;
		ec = vec_info(v, &vi);
		if (!is_ok(ec)) {
				err.code = ec;
				err.b = v.dim();
				return err;
		}
		if (ui.len != 3 || vi.len != 3) {
				err.code = ErrorCode::InvalidDimension;
				err.a = u.dim();
				err.b = v.dim();
				return err;
		}
		if (u.dim().rows != out.dim().rows || u.dim().cols != out.dim().cols)
				return {ErrorCode::DimensionMismatch, u.dim(), out.dim()};
		if (!out.data)
				return {ErrorCode::Internal};
		if (out.stride < out.cols)
				return err_invalid_dim(out.dim());

		VecInfo oi = ui; // output orientation matches u
		ec = cross_raw(u, ui, v, vi, out, oi);
		if (!is_ok(ec))
				return {ec};

		if (expl) {
				expl->kind = Explanation::Kind::Cross;
				expl->input = u;
				expl->other = v;
				expl->result = out.view();
		}

		return err;
}

#if MATRIX_CORE_ENABLE_PROJECTION
Error op_proj_decompose_u_onto_v(In MatrixView u,
        In MatrixView v,
        Out MatrixMutView out_proj,
        Out MatrixMutView out_orth,
        Out ProjDecomposeResult* out,
        Out Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		Error err;
		VecInfo ui;
		ErrorCode ec = vec_info(u, &ui);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = u.dim();
				return err;
		}
		VecInfo vi;
		ec = vec_info(v, &vi);
		if (!is_ok(ec)) {
				err.code = ec;
				err.b = v.dim();
				return err;
		}
		if (ui.len != vi.len)
				return err_dim_mismatch(u.dim(), v.dim());
		if (ui.len > kMaxRows)
				return err_invalid_dim(u.dim());
		if (u.dim().rows != out_proj.dim().rows || u.dim().cols != out_proj.dim().cols)
				return err_dim_mismatch(u.dim(), out_proj.dim());
		if (u.dim().rows != out_orth.dim().rows || u.dim().cols != out_orth.dim().cols)
				return err_dim_mismatch(u.dim(), out_orth.dim());

		if (!out_proj.data || !out_orth.data)
				return {ErrorCode::Internal};
		if (out_proj.stride < out_proj.cols || out_orth.stride < out_orth.cols)
				return err_invalid_dim(out_proj.dim());

		ProjDecomposeResult res;
		ec = proj_decompose_raw_impl(u, ui, v, vi, out_proj, out_orth, res);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = u.dim();
				err.b = v.dim();
				return err;
		}
		if (out)
				*out = res;
		if (expl) {
				expl->kind = Explanation::Kind::Projection;
				expl->input = u;
				expl->other = v;
				expl->result = out_proj.view();
				expl->value = res.dot_uv;
				expl->dot_vv = res.dot_vv;
				expl->k = res.k;
				expl->proj_norm2 = res.proj_norm2;
				expl->orth_norm2 = res.orth_norm2;
		}

		return err;
}
#else
Error op_proj_decompose_u_onto_v(In MatrixView u,
        In MatrixView v,
        Out MatrixMutView out_proj,
        Out MatrixMutView out_orth,
        Out ProjDecomposeResult* out,
        Out Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		(void)u;
		(void)v;
		(void)out_proj;
		(void)out_orth;
		(void)out;
		return err_feature_disabled();
}
#endif

} // namespace matrix_core
