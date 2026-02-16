#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/row_reduction.hpp"

#include <new>

namespace matrix_core {
namespace {

struct EchelonCtx {
		MatrixView input;
		EchelonKind kind = EchelonKind::Rref;
		std::size_t op_count = 0;
};

std::size_t echelon_step_count(const void* vctx) noexcept {
		const auto* ctx = static_cast<const EchelonCtx*>(vctx);
		return ctx->op_count + 1;
}

ErrorCode echelon_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept {
		const auto* ctx = static_cast<const EchelonCtx*>(vctx);
		if (!ctx->input.data)
				return ErrorCode::Internal;
		if (!out.scratch)
				return ErrorCode::Internal;

		if (out.caption && out.caption_cap)
				out.caption[0] = '\0';
		if (out.latex && out.latex_cap)
				out.latex[0] = '\0';

		if (index == 0)
				return latex::write_matrix_display(ctx->input, latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});

		MatrixMutView work;
		ErrorCode ec = matrix_clone(*out.scratch, ctx->input, &work);
		if (!is_ok(ec))
				return ec;

		OpObserver obs;
		obs.target = index; // stop after applying index ops
		ec = echelon_apply(work, ctx->kind, &obs);
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

constexpr ExplanationVTable kEchelonVTable = {
        .step_count = &echelon_step_count,
        .render_step = &echelon_render_step,
        .destroy = nullptr,
};

} // namespace

Error op_echelon(MatrixView a, EchelonKind kind, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept {
		Error err;
		if (out.rows != a.rows || out.cols != a.cols)
				return {ErrorCode::DimensionMismatch, a.dim(), out.dim()};

		ErrorCode ec = matrix_copy(a, out);
		if (!is_ok(ec))
				return {ec};

		OpObserver obs;
		obs.target = static_cast<std::size_t>(-1);
		ec = echelon_apply(out, kind, &obs);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = a.dim();
				return err;
		}

		if (opts.enable) {
				if (!opts.persist || !expl)
						return {ErrorCode::Internal};

				ArenaScope tx(*opts.persist);
				void* mem = opts.persist->allocate(sizeof(EchelonCtx), alignof(EchelonCtx));
				if (!mem)
						return {ErrorCode::Overflow};
				auto* ctx = new (mem) EchelonCtx{};
				ctx->input = a;
				ctx->kind = kind;
				ctx->op_count = obs.count;
				*expl = Explanation::make(ctx, &kEchelonVTable);
				tx.commit();
		}

		return err;
}

} // namespace matrix_core
