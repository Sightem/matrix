#include "matrix_core/ops.hpp"

#if MATRIX_CORE_ENABLE_MINOR_MATRIX
#include "matrix_core/det_detail.hpp"

namespace matrix_core {
namespace {
ErrorCode build_submatrix(MatrixView a, std::uint8_t del_r, std::uint8_t del_c, Arena& arena, MatrixMutView* out) noexcept {
		if (a.rows != a.cols)
				return ErrorCode::NotSquare;
		if (a.rows <= 1)
				return ErrorCode::InvalidDimension;
		if (del_r >= a.rows || del_c >= a.cols)
				return ErrorCode::IndexOutOfRange;

		MatrixMutView sub;
		ErrorCode ec = matrix_alloc(arena, static_cast<std::uint8_t>(a.rows - 1), static_cast<std::uint8_t>(a.cols - 1), &sub);
		if (!is_ok(ec))
				return ec;

		std::uint8_t dest_row = 0;
		for (std::uint8_t row = 0; row < a.rows; row++) {
				if (row == del_r)
						continue;
				std::uint8_t dest_col = 0;
				for (std::uint8_t col = 0; col < a.cols; col++) {
						if (col == del_c)
								continue;
						sub.at_mut(dest_row, dest_col) = a.at(row, col);
						dest_col++;
				}
				dest_row++;
		}

		*out = sub;
		return ErrorCode::Ok;
}
} // namespace

Error op_minor_matrix(MatrixView a, Arena& scratch, MatrixMutView out) noexcept {
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		if (a.rows <= 1) {
				Error err;
				err.code = ErrorCode::InvalidDimension;
				err.a = a.dim();
				return err;
		}

		if (out.rows != a.rows || out.cols != a.cols) {
				Error err;
				err.code = ErrorCode::DimensionMismatch;
				err.a = a.dim();
				err.b = out.dim();
				return err;
		}

		for (std::uint8_t i = 0; i < a.rows; i++) {
				for (std::uint8_t j = 0; j < a.cols; j++) {
						ArenaScratchScope elem_scope(scratch);

						MatrixMutView sub;
						ErrorCode ec = build_submatrix(a, i, j, scratch, &sub);
						if (!is_ok(ec)) {
								Error err;
								err.code = ec;
								err.a = a.dim();
								err.i = i;
								err.j = j;
								return err;
						}

						Rational det = Rational::from_int(0);
						std::size_t op_count = 0;
						ec = detail::det_elim(sub, &det, &op_count, static_cast<std::size_t>(-1), nullptr);
						if (!is_ok(ec)) {
								Error err;
								err.code = ec;
								err.a = a.dim();
								err.i = i;
								err.j = j;
								return err;
						}

						out.at_mut(i, j) = det;
				}
		}

	return {};
}
} // namespace matrix_core
#else

namespace matrix_core {
Error op_minor_matrix(MatrixView a, Arena& scratch, MatrixMutView out) noexcept {
		(void)a;
		(void)scratch;
		(void)out;
		return err_feature_disabled();
}
} // namespace matrix_core

#endif
