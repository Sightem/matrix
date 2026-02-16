#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/writer.hpp"

#include <new>

namespace matrix_core {
namespace {
struct VecInfo {
		std::uint8_t len = 0;
		bool is_row = false;
};

ErrorCode vec_info(In const MatrixView& m, Out VecInfo* out) noexcept {
		if (!out)
				return ErrorCode::Internal;
		if (!m.data)
				return ErrorCode::Internal;

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
		if (!out)
				return ErrorCode::Internal;

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

		if (ui_out)
				*ui_out = ui;
		if (vi_out)
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
		if (out.dot_vv.is_zero())
				return ErrorCode::DivisionByZero;

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

ErrorCode proj_decompose_raw(In const MatrixView& u,
        In const VecInfo& ui,
        In const MatrixView& v,
        In const VecInfo& vi,
        Out MatrixMutView& out_proj,
        Out MatrixMutView& out_orth,
        Out ProjDecomposeResult* out) noexcept {
		if (out)
				return proj_decompose_raw_impl(u, ui, v, vi, out_proj, out_orth, *out);

		ProjDecomposeResult local;
		return proj_decompose_raw_impl(u, ui, v, vi, out_proj, out_orth, local);
}

#endif // MATRIX_CORE_ENABLE_PROJECTION

struct DotCtx {
		MatrixView u;
		MatrixView v;
		VecInfo ui{};
		VecInfo vi{};
		Rational dot = Rational::from_int(0);
};

std::size_t dot_step_count(const void*) noexcept {
		// One compact computation step.
		return 1;
}

ErrorCode dot_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept {
		const auto* ctx = static_cast<const DotCtx*>(vctx);
		if (index != 0)
				return ErrorCode::StepOutOfRange;

		if (out.caption && out.caption_cap)
				out.caption[0] = '\0';
		if (out.latex && out.latex_cap)
				out.latex[0] = '\0';

		Writer w{out.latex, out.latex_cap, 0};
		ErrorCode ec = w.append("$$u\\cdot v = ");
		if (!is_ok(ec))
				return ec;

		for (std::uint8_t i = 0; i < ctx->ui.len; i++) {
				if (i != 0) {
						ec = w.append(" + ");
						if (!is_ok(ec))
								return ec;
				}

				ec = w.put('(');
				if (!is_ok(ec))
						return ec;
				ec = w.append_rational_latex(vec_at(ctx->u, ctx->ui, i));
				if (!is_ok(ec))
						return ec;
				ec = w.append(")\\cdot(");
				if (!is_ok(ec))
						return ec;
				ec = w.append_rational_latex(vec_at(ctx->v, ctx->vi, i));
				if (!is_ok(ec))
						return ec;
				ec = w.put(')');
				if (!is_ok(ec))
						return ec;
		}

		ec = w.append(" = ");
		if (!is_ok(ec))
				return ec;
		ec = w.append_rational_latex(ctx->dot);
		if (!is_ok(ec))
				return ec;
		return w.append("$$");
}

constexpr ExplanationVTable kDotVTable = {
        .step_count = &dot_step_count,
        .render_step = &dot_render_step,
        .destroy = nullptr,
};

struct CrossCtx {
		MatrixView u;
		MatrixView v;
		VecInfo ui{};
		VecInfo vi{};
		VecInfo oi{};
		MatrixView out_shape; // dimensions only (data ignored)
};

std::size_t cross_step_count(const void*) noexcept {
		// 0: formula, 1: result vector
		return 2;
}

ErrorCode cross_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept {
		const auto* ctx = static_cast<const CrossCtx*>(vctx);
		if (index >= 2)
				return ErrorCode::StepOutOfRange;
		if (!out.scratch)
				return ErrorCode::Internal;

		if (out.caption && out.caption_cap)
				out.caption[0] = '\0';
		if (out.latex && out.latex_cap)
				out.latex[0] = '\0';

		if (index == 0) {
				Writer w{out.latex, out.latex_cap, 0};
				ErrorCode ec = w.append(
				        "$$u\\times v = \\begin{bmatrix}u_2 v_3 - u_3 v_2 \\\\ u_3 v_1 - u_1 v_3 \\\\ u_1 v_2 - u_2 v_1\\end{bmatrix}$$");
				return ec;
		}

		MatrixMutView tmp;
		ErrorCode ec = matrix_alloc(*out.scratch, ctx->out_shape.rows, ctx->out_shape.cols, &tmp);
		if (!is_ok(ec))
				return ec;

		ec = cross_raw(ctx->u, ctx->ui, ctx->v, ctx->vi, tmp, ctx->oi);
		if (!is_ok(ec))
				return ec;

		if (out.caption && out.caption_cap) {
				Writer w{out.caption, out.caption_cap, 0};
				ec = w.append("$u\\times v$");
				if (!is_ok(ec))
						return ec;
		}

		return latex::write_matrix_display(tmp.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

constexpr ExplanationVTable kCrossVTable = {
        .step_count = &cross_step_count,
        .render_step = &cross_render_step,
        .destroy = nullptr,
};

#if MATRIX_CORE_ENABLE_PROJECTION
struct ProjCtx {
		MatrixView u;
		MatrixView v;
		VecInfo ui{};
		VecInfo vi{};
		MatrixView out_shape; // dimensions only (data ignored)
		ProjDecomposeResult res{};
};

std::size_t proj_step_count(const void*) noexcept {
		// formula, u . v, v . v, k, proj vec, orth vec, ||proj||^2, ||orth||^2
		return 8;
}

ErrorCode proj_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept {
		const auto* ctx = static_cast<const ProjCtx*>(vctx);
		if (index >= proj_step_count(nullptr))
				return ErrorCode::StepOutOfRange;
		if (!out.scratch)
				return ErrorCode::Internal;

		if (out.caption && out.caption_cap)
				out.caption[0] = '\0';
		if (out.latex && out.latex_cap)
				out.latex[0] = '\0';

		ErrorCode ec = ErrorCode::Ok;
		Writer latex_w{out.latex, out.latex_cap, 0};

		if (index == 0) {
				return latex_w.append("$$proj_v(u) = \\frac{u\\cdot v}{v\\cdot v} v$$");
		}

		if (index == 1) {
				ec = latex_w.append("$$u\\cdot v = ");
				if (!is_ok(ec))
						return ec;
				ec = latex_w.append_rational_latex(ctx->res.dot_uv);
				if (!is_ok(ec))
						return ec;
				return latex_w.append("$$");
		}

		if (index == 2) {
				ec = latex_w.append("$$v\\cdot v = ");
				if (!is_ok(ec))
						return ec;
				ec = latex_w.append_rational_latex(ctx->res.dot_vv);
				if (!is_ok(ec))
						return ec;
				return latex_w.append("$$");
		}

		if (index == 3) {
				ec = latex_w.append("$$k = \\frac{u\\cdot v}{v\\cdot v} = ");
				if (!is_ok(ec))
						return ec;
				ec = latex_w.append_rational_latex(ctx->res.k);
				if (!is_ok(ec))
						return ec;
				return latex_w.append("$$");
		}

		if (index == 6) {
				ec = latex_w.append("$$||proj||^2 = ");
				if (!is_ok(ec))
						return ec;
				ec = latex_w.append_rational_latex(ctx->res.proj_norm2);
				if (!is_ok(ec))
						return ec;
				return latex_w.append("$$");
		}

		if (index == 7) {
				ec = latex_w.append("$$||orth||^2 = ");
				if (!is_ok(ec))
						return ec;
				ec = latex_w.append_rational_latex(ctx->res.orth_norm2);
				if (!is_ok(ec))
						return ec;
				return latex_w.append("$$");
		}

		if (index == 4) {
				MatrixMutView proj;
				ec = matrix_alloc(*out.scratch, ctx->out_shape.rows, ctx->out_shape.cols, &proj);
				if (!is_ok(ec))
						return ec;

				// proj = k * v
				for (std::uint8_t i = 0; i < ctx->ui.len; i++) {
						Rational term;
						ec = rational_mul(ctx->res.k, vec_at(ctx->v, ctx->vi, i), &term);
						if (!is_ok(ec))
								return ec;
						vec_at_mut(proj, ctx->ui, i) = term;
				}

				if (out.caption && out.caption_cap) {
						Writer caption_w{out.caption, out.caption_cap, 0};
						ec = caption_w.append("$proj = kv$");
						if (!is_ok(ec))
								return ec;
				}
				return latex::write_matrix_display(proj.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
		}

		if (index == 5) {
				MatrixMutView orth;
				ec = matrix_alloc(*out.scratch, ctx->out_shape.rows, ctx->out_shape.cols, &orth);
				if (!is_ok(ec))
						return ec;

				// orth = u - proj = u - k*v
				for (std::uint8_t i = 0; i < ctx->ui.len; i++) {
						Rational kvi;
						ec = rational_mul(ctx->res.k, vec_at(ctx->v, ctx->vi, i), &kvi);
						if (!is_ok(ec))
								return ec;
						Rational term;
						ec = rational_sub(vec_at(ctx->u, ctx->ui, i), kvi, &term);
						if (!is_ok(ec))
								return ec;
						vec_at_mut(orth, ctx->ui, i) = term;
				}

				if (out.caption && out.caption_cap) {
						Writer caption_w{out.caption, out.caption_cap, 0};
						ec = caption_w.append("$orth = u - proj$");
						if (!is_ok(ec))
								return ec;
				}
				return latex::write_matrix_display(orth.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
		}

		return ErrorCode::Internal;
}

constexpr ExplanationVTable kProjVTable = {
        .step_count = &proj_step_count,
        .render_step = &proj_render_step,
        .destroy = nullptr,
};
#endif

template <typename Ctx> ErrorCode ctx_alloc(Arena& arena, Ctx** out) noexcept {
		void* mem = arena.allocate(sizeof(Ctx), alignof(Ctx));
		if (!mem)
				return ErrorCode::Overflow;
		*out = new (mem) Ctx{};
		return ErrorCode::Ok;
}

} // namespace

Error op_dot(In MatrixView u, In MatrixView v, Out Rational* out, Out Explanation* expl, In const ExplainOptions& opts) noexcept {
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

		if (opts.enable) {
				if (!opts.persist || !expl)
						return {ErrorCode::Internal};
				ArenaScope tx(*opts.persist);
				DotCtx* ctx = nullptr;
				ec = ctx_alloc(*opts.persist, &ctx);
				if (!is_ok(ec))
						return {ec};
				ctx->u = u;
				ctx->v = v;
				ctx->ui = ui;
				ctx->vi = vi;
				ctx->dot = dot;
				*expl = Explanation::make(ctx, &kDotVTable);
				tx.commit();
		}

		return err;
}

Error op_cross(In MatrixView u, In MatrixView v, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept {
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

		VecInfo oi = ui; // output orientation matches u
		ec = cross_raw(u, ui, v, vi, out, oi);
		if (!is_ok(ec))
				return {ec};

		if (opts.enable) {
				if (!opts.persist || !expl)
						return {ErrorCode::Internal};
				ArenaScope tx(*opts.persist);
				CrossCtx* ctx = nullptr;
				ec = ctx_alloc(*opts.persist, &ctx);
				if (!is_ok(ec))
						return {ec};
				ctx->u = u;
				ctx->v = v;
				ctx->ui = ui;
				ctx->vi = vi;
				ctx->oi = oi;
				ctx->out_shape = out.view();
				*expl = Explanation::make(ctx, &kCrossVTable);
				tx.commit();
		}

		return err;
}

#if MATRIX_CORE_ENABLE_PROJECTION
Error op_proj_decompose_u_onto_v(In MatrixView u,
        In MatrixView v,
        Out MatrixMutView out_proj,
        Out MatrixMutView out_orth,
        Out ProjDecomposeResult* out,
        Out Explanation* expl,
        In const ExplainOptions& opts) noexcept {
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
		if (u.dim().rows != out_proj.dim().rows || u.dim().cols != out_proj.dim().cols)
				return err_dim_mismatch(u.dim(), out_proj.dim());
		if (u.dim().rows != out_orth.dim().rows || u.dim().cols != out_orth.dim().cols)
				return err_dim_mismatch(u.dim(), out_orth.dim());

		if (opts.enable) {
				if (!opts.persist || !expl)
						return {ErrorCode::Internal};
				ArenaScope tx(*opts.persist);
				ProjCtx* ctx = nullptr;
				ec = ctx_alloc(*opts.persist, &ctx);
				if (!is_ok(ec))
						return {ec};
				ctx->u = u;
				ctx->v = v;
				ctx->ui = ui;
				ctx->vi = vi;
				ctx->out_shape = out_proj.view();

				ec = proj_decompose_raw(u, ui, v, vi, out_proj, out_orth, &ctx->res);
				if (!is_ok(ec)) {
						err.code = ec;
						err.a = u.dim();
						err.b = v.dim();
						return err;
				}
				if (out)
						*out = ctx->res;

				*expl = Explanation::make(ctx, &kProjVTable);
				tx.commit();
				return err;
		}

		ec = proj_decompose_raw(u, ui, v, vi, out_proj, out_orth, out);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = u.dim();
				err.b = v.dim();
				return err;
		}

		return err;
}
#else
Error op_proj_decompose_u_onto_v(In MatrixView u,
        In MatrixView v,
        Out MatrixMutView out_proj,
        Out MatrixMutView out_orth,
        Out ProjDecomposeResult* out,
        Out Explanation* expl,
        In const ExplainOptions& opts) noexcept {
		(void)u;
		(void)v;
		(void)out_proj;
		(void)out_orth;
		(void)out;
		(void)expl;
		(void)opts;
		return err_feature_disabled();
}
#endif

} // namespace matrix_core
