#include "matrix_core/explanation.hpp"
#include "matrix_core/config.hpp"

namespace matrix_core {
std::size_t Explanation::step_count() const noexcept {
		switch (kind) {
		case Kind::None: return 0;
		case Kind::Dot: return 1;
		case Kind::Projection: return 8;
		case Kind::Ref:
		case Kind::Rref:
		case Kind::Inverse: return op_count + 1;
		case Kind::Det:
		case Kind::DetReplace: return op_count + 2;
		case Kind::Cofactor: return op_count + (input.rows <= 1 ? 2 : 3);
		case Kind::Add:
		case Kind::Sub:
		case Kind::Mul:
		case Kind::Transpose:
		case Kind::Cross: return 2;
		}
		return 0;
}

ErrorCode Explanation::render_step(std::size_t index, const StepRenderBuffers& out) const noexcept {
		if (!available())
				return ErrorCode::Internal;
		if (index >= step_count())
				return ErrorCode::StepOutOfRange;
		if (out.caption && out.caption_cap)
				out.caption[0] = '\0';
		if (out.latex && out.latex_cap)
				out.latex[0] = '\0';
		switch (kind) {
		case Kind::None: return ErrorCode::Internal;
		case Kind::Add:
		case Kind::Sub:
		case Kind::Mul: return render_binary(index, out);
		case Kind::Transpose: return render_transpose(index, out);
		case Kind::Ref:
		case Kind::Rref: return render_echelon(index, out);
		case Kind::Det: return render_det(index, out);
		case Kind::Inverse: return render_inverse(index, out);
		case Kind::Dot: return render_dot(index, out);
		case Kind::Cross: return render_cross(index, out);
		case Kind::DetReplace:
#if MATRIX_CORE_ENABLE_CRAMER
				return render_det_replace(index, out);
#else
				break;
#endif
		case Kind::Cofactor:
#if MATRIX_CORE_ENABLE_COFACTOR
				return render_cofactor(index, out);
#else
				break;
#endif
		case Kind::Projection:
#if MATRIX_CORE_ENABLE_PROJECTION
				return render_proj(index, out);
#else
				break;
#endif
		}
		return ErrorCode::FeatureDisabled;
}
} // namespace matrix_core
