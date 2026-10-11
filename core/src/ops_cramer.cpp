#include "matrix_core/ops.hpp"

#if MATRIX_CORE_ENABLE_CRAMER

namespace matrix_core {

Error op_cramer_solve(MatrixView a, MatrixView b, MatrixMutView work, MatrixMutView x_out) noexcept {
		Error err;
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		if (b.rows != a.rows || b.cols != 1)
				return err_dim_mismatch(a.dim(), b.dim());
		if (x_out.rows != a.rows || x_out.cols != 1)
				return err_dim_mismatch(a.dim(), x_out.dim());
		if (!x_out.data)
				return {ErrorCode::Internal};
		if (x_out.stride < x_out.cols)
				return err_invalid_dim(x_out.dim());

		Rational delta;
		err = op_det(a, work, &delta, nullptr);
		if (!is_ok(err)) {
				err.a = a.dim();
				return err;
		}
		if (delta.is_zero())
				return err_singular(a.dim());

		for (std::uint8_t col = 0; col < a.cols; col++) {
				Rational delta_i;
				err = op_det_replace_column(a, b, col, work, &delta_i, nullptr);
				if (!is_ok(err)) {
						err.a = a.dim();
						return err;
				}

				Rational x_i;
				ErrorCode ec = rational_div(delta_i, delta, &x_i);
				if (!is_ok(ec)) {
						err.code = ec;
						err.a = a.dim();
						return err;
				}

				x_out.at_mut(col, 0) = x_i;
		}

		return err;
}

} // namespace matrix_core
#endif
