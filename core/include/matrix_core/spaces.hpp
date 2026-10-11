#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/config.hpp"
#include "matrix_core/error.hpp"
#include "matrix_core/explanation.hpp"
#include "matrix_core/matrix.hpp"

namespace matrix_core {

// pivot / rank information derived from an RREF matrix (over a field)
//
// var_cols is the number of columns considered "variables" when the RREF comes 
// from an augmented matrix; e.g. for [A|b] it would be A.cols
struct SpaceInfo {
		std::uint8_t rank = 0;
		std::uint8_t nullity = 0;
		std::uint32_t pivot_mask = 0; // LSB = col 1 (col 0 is bit 0)

		// Valid entries are [0..rank).
		std::uint8_t pivot_cols[kMaxCols]{};

		// For each col in [0..var_cols), gives the pivot row index or 0xFF if free.
		std::uint8_t pivot_row_for_col[kMaxCols]{};
};

ErrorCode space_info_from_rref(In MatrixView rref, In std::uint8_t var_cols, Out SpaceInfo* out) noexcept;

// pivot columns of A form a basis for Col(A)
ErrorCode space_col_basis(In MatrixView a, In const SpaceInfo& info, Out MatrixMutView storage, Out MatrixMutView* out_basis) noexcept;

// non zero rows of RREF(A) form a basis for Row(A)
ErrorCode space_row_basis(In MatrixView rref, In std::uint8_t var_cols, In const SpaceInfo& info, Out MatrixMutView storage, Out MatrixMutView* out_basis) noexcept;

// construct a basis for Null(A) from RREF(A)
ErrorCode space_null_basis(In MatrixView rref, In std::uint8_t var_cols, In const SpaceInfo& info, Out MatrixMutView storage, Out MatrixMutView* out_basis) noexcept;

struct SolveResult {
		MatrixView particular;
		MatrixView null_basis;
		SpaceInfo info;
};

Error op_solve_rref(In MatrixView a, In MatrixView b, Out MatrixMutView saved_input, Out MatrixMutView work,
        Out MatrixMutView particular_storage, Out MatrixMutView basis_storage,
        Out SolveResult* out, Out Explanation* expl) noexcept;

} // namespace matrix_core

