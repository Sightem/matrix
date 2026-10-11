#include "matrix_core/spaces.hpp"

#include "matrix_core/ops.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/row_reduction.hpp"

namespace matrix_core {
namespace {
constexpr std::uint8_t kNoPivot = 0xFF;

std::uint8_t find_pivot_col(MatrixView rref, std::uint8_t row, std::uint8_t var_cols) noexcept {
		for (std::uint8_t c = 0; c < var_cols; c++) {
				if (!rref.at(row, c).is_zero())
						return c;
		}
		return kNoPivot;
}

} // namespace

ErrorCode space_info_from_rref(MatrixView rref, std::uint8_t var_cols, SpaceInfo* out) noexcept {
		if (!out)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && !rref.data)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && rref.stride < rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > kMaxCols)
				return ErrorCode::InvalidDimension;

		SpaceInfo info{};
		for (std::uint8_t c = 0; c < kMaxCols; c++)
				info.pivot_row_for_col[c] = kNoPivot;

		for (std::uint8_t r = 0; r < rref.rows; r++) {
				const std::uint8_t pc = find_pivot_col(rref, r, var_cols);
				if (pc == kNoPivot)
						continue;
				info.pivot_row_for_col[pc] = r;
				info.pivot_cols[info.rank++] = pc;
				info.pivot_mask |= (1u << pc);
				if (info.rank >= var_cols)
						break;
		}

		info.nullity = static_cast<std::uint8_t>(var_cols - info.rank);
		*out = info;
		return ErrorCode::Ok;
}

ErrorCode space_col_basis(MatrixView a, const SpaceInfo& info, MatrixMutView storage, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (a.rows && a.cols && !a.data)
				return ErrorCode::Internal;
		if (a.rows && a.cols && a.stride < a.cols)
				return ErrorCode::InvalidDimension;

		const std::uint8_t rank = info.rank;

		MatrixMutView basis{};
		ErrorCode ec = matrix_work_view(storage, a.rows, rank, &basis);
		if (!is_ok(ec))
				return ec;

		for (std::uint8_t k = 0; k < rank; k++) {
				const std::uint8_t pc = info.pivot_cols[k];
				for (std::uint8_t r = 0; r < a.rows; r++)
						basis.at_mut(r, k) = a.at(r, pc);
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

ErrorCode space_row_basis(MatrixView rref, std::uint8_t var_cols, const SpaceInfo& info, MatrixMutView storage, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && !rref.data)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && rref.stride < rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > rref.cols)
				return ErrorCode::InvalidDimension;

		const std::uint8_t rank = info.rank;

		MatrixMutView basis{};
		ErrorCode ec = matrix_work_view(storage, rank, var_cols, &basis);
		if (!is_ok(ec))
				return ec;

		for (std::uint8_t rr = 0; rr < rank; rr++) {
				for (std::uint8_t c = 0; c < var_cols; c++)
						basis.at_mut(rr, c) = rref.at(rr, c);
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

ErrorCode space_null_basis(MatrixView rref, std::uint8_t var_cols, const SpaceInfo& info, MatrixMutView storage, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && !rref.data)
				return ErrorCode::Internal;
		if (rref.rows && rref.cols && rref.stride < rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > kMaxCols)
				return ErrorCode::InvalidDimension;

		MatrixMutView basis{};
		ErrorCode ec = matrix_work_view(storage, var_cols, info.nullity, &basis);
		if (!is_ok(ec))
				return ec;
		matrix_fill_zero(basis);

		std::uint8_t k = 0;
		for (std::uint8_t fc = 0; fc < var_cols; fc++) {
				if (info.pivot_row_for_col[fc] != kNoPivot)
						continue;
				basis.at_mut(fc, k) = Rational::from_int(1);
				for (std::uint8_t pi = 0; pi < info.rank; pi++) {
						const std::uint8_t pc = info.pivot_cols[pi];
						const std::uint8_t pr = info.pivot_row_for_col[pc];
						Rational neg{};
						ErrorCode ec2 = rational_neg(rref.at(pr, fc), &neg);
						if (!is_ok(ec2))
								return ec2;
						basis.at_mut(pc, k) = neg;
				}
				k++;
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

Error op_solve_rref(MatrixView a, MatrixView b, MatrixMutView saved_input, MatrixMutView work,
        MatrixMutView particular_storage, MatrixMutView basis_storage, SolveResult* out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		if (!out)
				return {ErrorCode::Internal};
		if (b.rows != a.rows || b.cols != 1)
				return err_dim_mismatch(a.dim(), b.dim());
		if (a.cols > kMaxCols)
				return err_invalid_dim(a.dim());
		if ((a.rows && a.cols && !a.data) || (b.rows && !b.data))
				return {ErrorCode::Internal};
		if ((a.rows && a.cols && a.stride < a.cols) || (b.rows && b.stride < b.cols))
				return {ErrorCode::InvalidDimension};

		const std::uint8_t n = a.cols;
		MatrixMutView rref;
		ErrorCode ec = matrix_work_view(work, a.rows, static_cast<std::uint8_t>(n + 1u), &rref);
		if (!is_ok(ec))
				return {ec};
		MatrixMutView aug = rref;
		if (expl) {
				ec = matrix_work_view(saved_input, rref.rows, rref.cols, &aug);
				if (!is_ok(ec))
						return {ec};
		}
		for (std::uint8_t r = 0; r < a.rows; r++) {
				for (std::uint8_t c = 0; c < n; c++)
						aug.at_mut(r, c) = a.at(r, c);
				aug.at_mut(r, n) = b.at(r, 0);
		}

		Explanation steps;
		if (expl) {
				Error err = op_echelon(aug.view(), EchelonKind::Rref, rref, &steps);
				if (!is_ok(err))
						return err;
		} else {
				ec = echelon_apply(rref, EchelonKind::Rref, nullptr);
				if (!is_ok(ec))
						return {ec};
		}
		for (std::uint8_t r = 0; r < rref.rows; r++) {
				if (find_pivot_col(rref.view(), r, n) == kNoPivot && !rref.at(r, n).is_zero())
						return {ErrorCode::NoSolution, a.dim(), b.dim()};
		}

		SpaceInfo info;
		ec = space_info_from_rref(rref.view(), n, &info);
		if (!is_ok(ec))
				return {ec};
		MatrixMutView particular;
		ec = matrix_work_view(particular_storage, n, 1, &particular);
		if (!is_ok(ec))
				return {ec};
		matrix_fill_zero(particular);
		for (std::uint8_t pi = 0; pi < info.rank; pi++) {
				const std::uint8_t pc = info.pivot_cols[pi];
				particular.at_mut(pc, 0) = rref.at(info.pivot_row_for_col[pc], n);
		}
		MatrixMutView null_basis;
		ec = space_null_basis(rref.view(), n, info, basis_storage, &null_basis);
		if (!is_ok(ec))
				return {ec};

		*out = {particular.view(), null_basis.view(), info};
		if (expl)
				*expl = steps;
		return {};
}

} // namespace matrix_core

