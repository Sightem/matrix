#include "matrix_core/row_reduction.hpp"

#include "matrix_core/ops.hpp"

namespace matrix_core {
namespace {
constexpr bool is_one(const Rational& r) noexcept { return r.num() == 1 && r.den() == 1; }
} // namespace

ErrorCode echelon_apply(MatrixMutView m, EchelonKind kind, OpObserver* obs) noexcept {
		const std::uint8_t rows = m.rows;
		const std::uint8_t cols = m.cols;

		std::uint8_t pivot_row = 0;
		for (std::uint8_t pivot_col = 0; pivot_col < cols && pivot_row < rows; pivot_col++) {
				// Find pivot row.
				std::uint8_t best_row = pivot_row;
				bool found = false;
				for (std::uint8_t row = pivot_row; row < rows; row++) {
						if (!m.at(row, pivot_col).is_zero()) {
								best_row = row;
								found = true;
								break;
						}
				}
				if (!found)
						continue;

				if (best_row != pivot_row) {
						apply_swap(m, pivot_row, best_row);
						if (obs) {
								RowOp op;
								op.kind = RowOpKind::Swap;
								op.target_row = pivot_row;
								op.source_row = best_row;
								if (!obs->on_op(op))
										return ErrorCode::Ok;
						}
				}

				// make pivot = 1 for RREF
				if (kind == EchelonKind::Rref) {
						const Rational pivot = m.at(pivot_row, pivot_col);
						// avoid emitting no op scale steps when the pivot is already 1
						if (!is_one(pivot)) {
								Rational inv;
								ErrorCode ec = rational_div(Rational::from_int(1), pivot, &inv);
								if (!is_ok(ec))
										return ec;
								ec = apply_scale(m, pivot_row, inv);
								if (!is_ok(ec))
										return ec;
								if (obs) {
										RowOp op;
										op.kind = RowOpKind::Scale;
										op.target_row = pivot_row;
										op.scalar = inv;
										if (!obs->on_op(op))
												return ErrorCode::Ok;
								}
						}
				} else {
						// REF: keep pivots "simple" but normalize sign so leading pivots are
						// positive. this avoids leaving a pivot at -1 (common after elimination),
						// which is surprising for users expecting a conventional REF
						const Rational pivot = m.at(pivot_row, pivot_col);

						if (pivot.num() < 0) {
								const Rational neg_one = Rational::from_int(-1);
								ErrorCode ec = apply_scale(m, pivot_row, neg_one);
								if (!is_ok(ec))
										return ec;
								if (obs) {
										RowOp op;
										op.kind = RowOpKind::Scale;
										op.target_row = pivot_row;
										op.scalar = neg_one;
										if (!obs->on_op(op))
												return ErrorCode::Ok;
								}
						}
				}

				// eliminate
				for (std::uint8_t row = 0; row < rows; row++) {
						if (row == pivot_row)
								continue;
						if (kind == EchelonKind::Ref && row < pivot_row)
								continue;

						const Rational entry = m.at(row, pivot_col);
						if (entry.is_zero())
								continue;

						Rational factor;
						ErrorCode ec = rational_neg(entry, &factor);
						if (!is_ok(ec))
								return ec;

						if (kind == EchelonKind::Ref) {
								const Rational pivot = m.at(pivot_row, pivot_col);
								ec = rational_div(factor, pivot, &factor);
								if (!is_ok(ec))
										return ec;
						}

						ec = apply_addmul(m, row, pivot_row, factor);
						if (!is_ok(ec))
								return ec;

						if (obs) {
								RowOp op;
								op.kind = RowOpKind::AddMul;
								op.target_row = row;
								op.source_row = pivot_row;
								op.scalar = factor;
								if (!obs->on_op(op))
										return ErrorCode::Ok;
						}
				}

				pivot_row++;
		}

		return ErrorCode::Ok;
}

} // namespace matrix_core

