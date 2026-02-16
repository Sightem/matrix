#include "matrix_core/matrix_core.hpp"

#include "test_dbg_ce.hpp"

#include <cassert>

#if defined(MATRIX_CE_TESTS)
#include <debug.h>
#endif

using matrix_core::Arena;
using matrix_core::EchelonKind;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::SpaceInfo;

static MatrixMutView mat2(Arena& arena, std::int64_t a00, std::int64_t a01, std::int64_t a10, std::int64_t a11) {
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(arena, 2, 2, &m) == ErrorCode::Ok);
		m.at_mut(0, 0) = Rational::from_int(a00);
		m.at_mut(0, 1) = Rational::from_int(a01);
		m.at_mut(1, 0) = Rational::from_int(a10);
		m.at_mut(1, 1) = Rational::from_int(a11);
		return m;
}

int main() {
#if defined(MATRIX_CE_TESTS)
#ifdef NDEBUG
		dbg_printf("[test_spaces] NDEBUG defined (asserts off)\n");
#else
		dbg_printf("[test_spaces] NDEBUG not defined (asserts on)\n");
#endif
#endif
		Slab slab;
		assert(slab.init(128 * 1024) == ErrorCode::Ok);
		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		// Full rank: I2
		{
				MatrixMutView a = mat2(persist, 1, 0, 0, 1);

				MatrixMutView rref;
				assert(matrix_core::matrix_alloc(persist, 2, 2, &rref) == ErrorCode::Ok);
				auto err = matrix_core::op_echelon(a.view(), EchelonKind::Rref, rref, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(matrix_core::is_ok(err));

				SpaceInfo info{};
				assert(matrix_core::space_info_from_rref(rref.view(), 2, &info) == ErrorCode::Ok);
				assert(info.rank == 2);
				assert(info.nullity == 0);
				assert((info.pivot_mask & 0x3u) == 0x3u);
				assert(info.pivot_cols[0] == 0);
				assert(info.pivot_cols[1] == 1);

				MatrixMutView nbas{};
				assert(matrix_core::space_null_basis(rref.view(), 2, info, persist, &nbas) == ErrorCode::Ok);
				assert(nbas.rows == 2 && nbas.cols == 1);
				assert(nbas.at(0, 0).is_zero());
				assert(nbas.at(1, 0).is_zero());

#if defined(MATRIX_CE_TESTS)
				matrix_test_ce::print_u64("rank", info.rank);
				matrix_test_ce::print_u64("nullity", info.nullity);
				matrix_test_ce::print_matrix("Null(A)", nbas.view());
#endif
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_spaces] after full-rank asserts\n");
#endif

		// Rank-1: [[1,1],[2,2]]
		{
				MatrixMutView a = mat2(persist, 1, 1, 2, 2);

				MatrixMutView rref;
				assert(matrix_core::matrix_alloc(persist, 2, 2, &rref) == ErrorCode::Ok);
				auto err = matrix_core::op_echelon(a.view(), EchelonKind::Rref, rref, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(matrix_core::is_ok(err));

				SpaceInfo info{};
				assert(matrix_core::space_info_from_rref(rref.view(), 2, &info) == ErrorCode::Ok);
				assert(info.rank == 1);
				assert(info.nullity == 1);
				assert(info.pivot_cols[0] == 0);

				MatrixMutView nbas{};
				assert(matrix_core::space_null_basis(rref.view(), 2, info, persist, &nbas) == ErrorCode::Ok);
				assert(nbas.rows == 2 && nbas.cols == 1);
				// Basis should be [-1, 1]^T.
				assert(nbas.at(0, 0).num() == -1 && nbas.at(0, 0).den() == 1);
				assert(nbas.at(1, 0).num() == 1 && nbas.at(1, 0).den() == 1);

				MatrixMutView rbas{};
				assert(matrix_core::space_row_basis(rref.view(), 2, info, persist, &rbas) == ErrorCode::Ok);
				assert(rbas.rows == 1 && rbas.cols == 2);
				assert(rbas.at(0, 0).num() == 1 && rbas.at(0, 0).den() == 1);
				assert(rbas.at(0, 1).num() == 1 && rbas.at(0, 1).den() == 1);

				MatrixMutView cbas{};
				assert(matrix_core::space_col_basis(a.view(), info, persist, &cbas) == ErrorCode::Ok);
				assert(cbas.rows == 2 && cbas.cols == 1);
				assert(cbas.at(0, 0).num() == 1 && cbas.at(0, 0).den() == 1);
				assert(cbas.at(1, 0).num() == 2 && cbas.at(1, 0).den() == 1);

#if defined(MATRIX_CE_TESTS)
				matrix_test_ce::print_u64("rank", info.rank);
				matrix_test_ce::print_u64("nullity", info.nullity);
				matrix_test_ce::print_matrix("Null(A)", nbas.view());
				matrix_test_ce::print_matrix("Row(A)", rbas.view());
				matrix_test_ce::print_matrix("Col(A)", cbas.view());
#endif
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_spaces] after rank-1 asserts\n");
#endif

		// var_cols smaller than rref.cols (augmented style)
		{
				MatrixMutView rref;
				assert(matrix_core::matrix_alloc(persist, 2, 3, &rref) == ErrorCode::Ok);
				matrix_core::matrix_fill_zero(rref);
				// [1 0 | 5]
				// [0 0 | 0]
				rref.at_mut(0, 0) = Rational::from_int(1);
				rref.at_mut(0, 2) = Rational::from_int(5);

				SpaceInfo info{};
				assert(matrix_core::space_info_from_rref(rref.view(), 2, &info) == ErrorCode::Ok);
				assert(info.rank == 1);
				assert(info.nullity == 1);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_spaces] after augmented asserts\n");
#endif

		return 0;
}
