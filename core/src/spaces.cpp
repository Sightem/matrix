#include "matrix_core/spaces.hpp"

#include "matrix_core/rational.hpp"

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

std::uint32_t pivot_mask_from_cols(const std::uint8_t* pivot_cols, std::uint8_t rank) noexcept {
		std::uint32_t mask = 0;
		for (std::uint8_t i = 0; i < rank; i++) {
				const std::uint8_t c = pivot_cols[i];
				if (c < 32)
						mask |= (1u << c);
		}
		return mask;
}
} // namespace

ErrorCode space_info_from_rref(MatrixView rref, std::uint8_t var_cols, SpaceInfo* out) noexcept {
		if (!out)
				return ErrorCode::Internal;
		if (!rref.data)
				return ErrorCode::Internal;
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
				if (info.pivot_row_for_col[pc] != kNoPivot)
						continue;
				info.pivot_row_for_col[pc] = r;
				info.pivot_cols[info.rank++] = pc;
				if (info.rank >= var_cols)
						break;
		}

		info.nullity = static_cast<std::uint8_t>(var_cols - info.rank);
		info.pivot_mask = pivot_mask_from_cols(info.pivot_cols, info.rank);
		*out = info;
		return ErrorCode::Ok;
}

ErrorCode space_col_basis(MatrixView a, const SpaceInfo& info, Arena& arena, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (!a.data)
				return ErrorCode::Internal;

		const std::uint8_t rank = info.rank;
		if (rank == 0) {
				MatrixMutView z{};
				ErrorCode ec = matrix_alloc(arena, a.rows, 1, &z);
				if (!is_ok(ec))
						return ec;
				matrix_fill_zero(z);
				*out_basis = z;
				return ErrorCode::Ok;
		}

		MatrixMutView basis{};
		ErrorCode ec = matrix_alloc(arena, a.rows, rank, &basis);
		if (!is_ok(ec))
				return ec;

		for (std::uint8_t k = 0; k < rank; k++) {
				const std::uint8_t pc = info.pivot_cols[k];
				if (pc >= a.cols)
						return ErrorCode::InvalidDimension;
				for (std::uint8_t r = 0; r < a.rows; r++)
						basis.at_mut(r, k) = a.at(r, pc);
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

ErrorCode space_row_basis(MatrixView rref, std::uint8_t var_cols, const SpaceInfo& info, Arena& arena, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (!rref.data)
				return ErrorCode::Internal;
		if (var_cols > rref.cols)
				return ErrorCode::InvalidDimension;

		const std::uint8_t rank = info.rank;
		if (rank == 0) {
				MatrixMutView z{};
				ErrorCode ec = matrix_alloc(arena, 1, var_cols, &z);
				if (!is_ok(ec))
						return ec;
				matrix_fill_zero(z);
				*out_basis = z;
				return ErrorCode::Ok;
		}

		MatrixMutView basis{};
		ErrorCode ec = matrix_alloc(arena, rank, var_cols, &basis);
		if (!is_ok(ec))
				return ec;

		for (std::uint8_t rr = 0; rr < rank; rr++) {
				for (std::uint8_t c = 0; c < var_cols; c++)
						basis.at_mut(rr, c) = rref.at(rr, c);
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

ErrorCode space_null_basis(MatrixView rref, std::uint8_t var_cols, const SpaceInfo& info, Arena& arena, MatrixMutView* out_basis) noexcept {
		if (!out_basis)
				return ErrorCode::Internal;
		if (!rref.data)
				return ErrorCode::Internal;
		if (var_cols > rref.cols)
				return ErrorCode::InvalidDimension;
		if (var_cols > kMaxCols)
				return ErrorCode::InvalidDimension;

		std::uint8_t free_cols[kMaxCols]{};
		std::uint8_t free_count = 0;
		for (std::uint8_t c = 0; c < var_cols; c++) {
				if (info.pivot_row_for_col[c] == kNoPivot)
						free_cols[free_count++] = c;
		}

		if (free_count == 0) {
				// Null space is {0}. show the zero vector (not a basis)
				MatrixMutView z{};
				ErrorCode ec = matrix_alloc(arena, var_cols, 1, &z);
				if (!is_ok(ec))
						return ec;
				matrix_fill_zero(z);
				*out_basis = z;
				return ErrorCode::Ok;
		}

		MatrixMutView basis{};
		ErrorCode ec = matrix_alloc(arena, var_cols, free_count, &basis);
		if (!is_ok(ec))
				return ec;
		matrix_fill_zero(basis);

		for (std::uint8_t k = 0; k < free_count; k++) {
				const std::uint8_t fc = free_cols[k];
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
		}

		*out_basis = basis;
		return ErrorCode::Ok;
}

} // namespace matrix_core

