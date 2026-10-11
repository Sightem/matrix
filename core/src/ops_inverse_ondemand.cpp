#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/row_reduction.hpp"
#include "matrix_core/writer.hpp"

#include <limits>

namespace matrix_core {
namespace {

constexpr bool is_one(const Rational& r) noexcept {
		return r.num() == 1 && r.den() == 1;
}

ErrorCode inverse_apply(MatrixMutView aug, std::uint8_t n, OpObserver& obs) noexcept {
		// aug is n x (2n): [A | I] -> [I | A^{-1}]

		for (std::uint8_t pivot = 0; pivot < n; pivot++) {
				// find pivot row
				std::uint8_t best_row = pivot;
				bool found = false;
				for (std::uint8_t row = pivot; row < n; row++) {
						if (!aug.at(row, pivot).is_zero()) {
								best_row = row;
								found = true;
								break;
						}
				}
				if (!found)
						return ErrorCode::Singular;

				// swap pivot row into place.
				if (best_row != pivot) {
						apply_swap(aug, pivot, best_row);
						RowOp op;
						op.kind = RowOpKind::Swap;
						op.target_row = pivot;
						op.source_row = best_row;
						if (!obs.on_op(op))
								return ErrorCode::Ok;
				}

				// scale pivot row to make pivot = 1.
				const Rational pivot_val = aug.at(pivot, pivot);
				if (!is_one(pivot_val)) {
						Rational inv;
						ErrorCode ec = rational_div(Rational::from_int(1), pivot_val, &inv);
						if (!is_ok(ec))
								return ec;
						ec = apply_scale(aug, pivot, inv);
						if (!is_ok(ec))
								return ec;
						RowOp op;
						op.kind = RowOpKind::Scale;
						op.target_row = pivot;
						op.scalar = inv;
						if (!obs.on_op(op))
								return ErrorCode::Ok;
				}

				// eliminate this column for all other rows.
				for (std::uint8_t row = 0; row < n; row++) {
						if (row == pivot)
								continue;

						const Rational entry = aug.at(row, pivot);
						if (entry.is_zero())
								continue;

						Rational factor;
						ErrorCode ec = rational_neg(entry, &factor);
						if (!is_ok(ec))
								return ec;

						ec = apply_addmul(aug, row, pivot, factor);
						if (!is_ok(ec))
								return ec;

						RowOp op;
						op.kind = RowOpKind::AddMul;
						op.target_row = row;
						op.source_row = pivot;
						op.scalar = factor;
						if (!obs.on_op(op))
								return ErrorCode::Ok;
				}
		}

		return ErrorCode::Ok;
}

ErrorCode build_augmented(MatrixView a, MatrixMutView storage, MatrixMutView* out_aug) noexcept {
		const std::uint8_t n = a.rows;
		const std::uint8_t cols = static_cast<std::uint8_t>(2u * n);
		MatrixMutView aug;
		ErrorCode ec = matrix_work_view(storage, n, cols, &aug);
		if (!is_ok(ec))
				return ec;
		matrix_fill_zero(aug);

		for (std::uint8_t row = 0; row < n; row++) {
				for (std::uint8_t col = 0; col < n; col++)
						aug.at_mut(row, col) = a.at(row, col);
				aug.at_mut(row, static_cast<std::uint8_t>(n + row)) = Rational::from_int(1);
		}

		*out_aug = aug;
		return ErrorCode::Ok;
}

} // namespace

ErrorCode Explanation::render_inverse(std::size_t index, const StepRenderBuffers& out) const noexcept {


		MatrixMutView aug;
		ErrorCode ec = build_augmented(input, out.work, &aug);
		if (!is_ok(ec))
				return ec;

		const std::uint8_t n = input.rows;
		const MatrixView left{n, n, aug.stride, aug.data};
		const MatrixView right{n, n, aug.stride, aug.data + n};

		if (index == 0)
				return latex::write_augmented_matrix_display(left, right, {out.latex, out.latex_cap});

		OpObserver obs;
		obs.target = index;
		ec = inverse_apply(aug, n, obs);
		if (!is_ok(ec))
				return ec;

		if (out.caption) {
				ec = row_op_caption(obs.last_op, out.caption, out.caption_cap);
				if (!is_ok(ec))
						return ec;
		}

		return latex::write_augmented_matrix_display(left, right, {out.latex, out.latex_cap});
}


Error op_inverse(MatrixView a, MatrixMutView storage, MatrixMutView out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		Error err;
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		if (out.rows != a.rows || out.cols != a.cols)
				return err_dim_mismatch(a.dim(), out.dim());
		if (a.rows == 0 || a.rows > std::numeric_limits<std::uint8_t>::max() / 2u)
				return err_invalid_dim(a.dim());
		if (!a.data || !out.data)
				return {ErrorCode::Internal};
		if (a.stride < a.cols || out.stride < out.cols)
				return err_invalid_dim(a.dim());

		MatrixMutView aug;
		ErrorCode ec = build_augmented(a, storage, &aug);
		if (!is_ok(ec)) {
				err.code = ec;
				err.a = a.dim();
				return err;
		}

		OpObserver obs;
		obs.target = static_cast<std::size_t>(-1);
		ec = inverse_apply(aug, a.rows, obs);
		if (!is_ok(ec)) {
				if (ec == ErrorCode::Singular)
						return err_singular(a.dim());
				err.code = ec;
				err.a = a.dim();
				return err;
		}

		const std::uint8_t n = a.rows;
		for (std::uint8_t row = 0; row < n; row++) {
				for (std::uint8_t col = 0; col < n; col++)
						out.at_mut(row, col) = aug.at(row, static_cast<std::uint8_t>(n + col));
		}

		if (expl) {
				expl->kind = Explanation::Kind::Inverse;
				expl->input = a;
				expl->op_count = obs.count;
		}

		return err;
}

} // namespace matrix_core
