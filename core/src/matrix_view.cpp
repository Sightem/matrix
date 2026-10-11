#include "matrix_core/matrix.hpp"

namespace matrix_core {

ErrorCode matrix_work_view(MatrixMutView storage, std::uint8_t rows, std::uint8_t cols, MatrixMutView* out) noexcept {
		if (!out)
				return ErrorCode::Internal;
		if (!rows || !cols) {
				*out = {rows, cols, storage.stride, nullptr};
				return ErrorCode::Ok;
		}
		if (rows > storage.rows || cols > storage.cols)
				return ErrorCode::Overflow;
		if (!storage.data)
				return ErrorCode::Internal;
		if (storage.stride < storage.cols)
				return ErrorCode::InvalidDimension;
		*out = {rows, cols, storage.stride, storage.data};
		return ErrorCode::Ok;
}

ErrorCode matrix_copy(MatrixView src, MatrixMutView dst) noexcept {
		if (src.rows != dst.rows || src.cols != dst.cols)
				return ErrorCode::DimensionMismatch;
		if (src.rows && src.cols && (!src.data || !dst.data))
				return ErrorCode::Internal;
		if (src.rows && src.cols && (src.stride < src.cols || dst.stride < dst.cols))
				return ErrorCode::InvalidDimension;

		for (std::uint8_t row = 0; row < src.rows; row++) {
				for (std::uint8_t col = 0; col < src.cols; col++)
						dst.at_mut(row, col) = src.at(row, col);
		}
		return ErrorCode::Ok;
}

void matrix_fill_zero(MatrixMutView m) noexcept {
		if (!m.data)
				return;
		for (std::uint8_t row = 0; row < m.rows; row++) {
				for (std::uint8_t col = 0; col < m.cols; col++)
						m.at_mut(row, col) = Rational::from_int(0);
		}
}

ErrorCode matrix_add(MatrixView a, MatrixView b, MatrixMutView out) noexcept {
		if (a.rows != b.rows || a.cols != b.cols)
				return ErrorCode::DimensionMismatch;
		if (out.rows != a.rows || out.cols != a.cols)
				return ErrorCode::DimensionMismatch;
		if (a.rows && a.cols && (!a.data || !b.data || !out.data))
				return ErrorCode::Internal;
		if (a.rows && a.cols && (a.stride < a.cols || b.stride < b.cols || out.stride < out.cols))
				return ErrorCode::InvalidDimension;

		for (std::uint8_t row = 0; row < a.rows; row++) {
				for (std::uint8_t col = 0; col < a.cols; col++) {
						Rational result;
						ErrorCode ec = rational_add(a.at(row, col), b.at(row, col), &result);
						if (!is_ok(ec))
								return ec;
						out.at_mut(row, col) = result;
				}
		}
		return ErrorCode::Ok;
}

ErrorCode matrix_sub(MatrixView a, MatrixView b, MatrixMutView out) noexcept {
		if (a.rows != b.rows || a.cols != b.cols)
				return ErrorCode::DimensionMismatch;
		if (out.rows != a.rows || out.cols != a.cols)
				return ErrorCode::DimensionMismatch;
		if (a.rows && a.cols && (!a.data || !b.data || !out.data))
				return ErrorCode::Internal;
		if (a.rows && a.cols && (a.stride < a.cols || b.stride < b.cols || out.stride < out.cols))
				return ErrorCode::InvalidDimension;

		for (std::uint8_t row = 0; row < a.rows; row++) {
				for (std::uint8_t col = 0; col < a.cols; col++) {
						Rational result;
						ErrorCode ec = rational_sub(a.at(row, col), b.at(row, col), &result);
						if (!is_ok(ec))
								return ec;
						out.at_mut(row, col) = result;
				}
		}
		return ErrorCode::Ok;
}

ErrorCode matrix_mul(MatrixView a, MatrixView b, MatrixMutView out) noexcept {
		if (a.cols != b.rows)
				return ErrorCode::DimensionMismatch;
		if (out.rows != a.rows || out.cols != b.cols)
				return ErrorCode::DimensionMismatch;
		if ((a.rows && a.cols && !a.data) || (b.rows && b.cols && !b.data) || (out.rows && out.cols && !out.data))
				return ErrorCode::Internal;
		if ((a.rows && a.cols && a.stride < a.cols) || (b.rows && b.cols && b.stride < b.cols) ||
		        (out.rows && out.cols && out.stride < out.cols))
				return ErrorCode::InvalidDimension;

		for (std::uint8_t i = 0; i < out.rows; i++) {
				for (std::uint8_t j = 0; j < out.cols; j++) {
						Rational sum = Rational::from_int(0);
						for (std::uint8_t k = 0; k < a.cols; k++) {
								Rational prod;
								ErrorCode ec = rational_mul(a.at(i, k), b.at(k, j), &prod);
								if (!is_ok(ec))
										return ec;
								Rational next;
								ec = rational_add(sum, prod, &next);
								if (!is_ok(ec))
										return ec;
								sum = next;
						}
						out.at_mut(i, j) = sum;
				}
		}
		return ErrorCode::Ok;
}

ErrorCode matrix_transpose(MatrixView a, MatrixMutView out) noexcept {
		if (out.rows != a.cols || out.cols != a.rows)
				return ErrorCode::DimensionMismatch;
		if (a.rows && a.cols && (!a.data || !out.data))
				return ErrorCode::Internal;
		if (a.rows && a.cols && (a.stride < a.cols || out.stride < out.cols))
				return ErrorCode::InvalidDimension;

		for (std::uint8_t row = 0; row < a.rows; row++) {
				for (std::uint8_t col = 0; col < a.cols; col++)
						out.at_mut(col, row) = a.at(row, col);
		}
		return ErrorCode::Ok;
}

} // namespace matrix_core
