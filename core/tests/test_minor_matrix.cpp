#include "matrix_core/matrix_core.hpp"

#include "test_dbg_ce.hpp"

#include <cassert>
#include <cstdint>

#if defined(MATRIX_CE_TESTS)
#include <debug.h>
#endif

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;

static MatrixMutView mat4(Arena& a) {
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(a, 4, 4, &m) == ErrorCode::Ok);
		return m;
}

int main() {
#if defined(MATRIX_CE_TESTS)
#ifdef NDEBUG
		dbg_printf("[test_minor_matrix] NDEBUG defined (asserts off)\n");
#else
		dbg_printf("[test_minor_matrix] NDEBUG not defined (asserts on)\n");
#endif
#endif
		Slab slab;
		assert(slab.init(256 * 1024) == ErrorCode::Ok);

		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		// A =
		// [ 4 -1  1  6 ]
		// [ 0  0 -3  3 ]
		// [ 4  1  0 14 ]
		// [ 4  1  3  2 ]
		MatrixMutView a = mat4(persist);
		const std::int64_t avals[4][4] = {
		        {4, -1, 1, 6},
		        {0, 0, -3, 3},
		        {4, 1, 0, 14},
		        {4, 1, 3, 2},
		};
		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++)
						a.at_mut(r, c) = Rational::from_int(avals[r][c]);
		}

		const std::int64_t expected_minor[4][4] = {
		        {-27, -108, 0, 0},
		        {72, -48, -96, 24},
		        {36, 24, -24, 24},
		        {63, -84, -24, 24},
		};

		MatrixMutView out = mat4(persist);
		const auto err = matrix_core::op_minor_matrix(a.view(), scratch, out);
		assert(matrix_core::is_ok(err));

#if defined(MATRIX_CE_TESTS)
		matrix_test_ce::print_matrix("minor(A)", out.view());
#endif

		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++) {
						const Rational v = out.at(r, c);
						assert(v.den() == 1);
						assert(v.num() == expected_minor[r][c]);
				}
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_minor_matrix] after expected minors asserts\n");
#endif

		// Argument validation.
		{
				MatrixMutView ns;
				assert(matrix_core::matrix_alloc(persist, 2, 3, &ns) == ErrorCode::Ok);
				MatrixMutView out2;
				assert(matrix_core::matrix_alloc(persist, 2, 2, &out2) == ErrorCode::Ok);
				const auto e = matrix_core::op_minor_matrix(ns.view(), scratch, out2);
				assert(!matrix_core::is_ok(e));
				assert(e.code == ErrorCode::NotSquare);
		}

		{
				MatrixMutView one;
				assert(matrix_core::matrix_alloc(persist, 1, 1, &one) == ErrorCode::Ok);
				MatrixMutView out1;
				assert(matrix_core::matrix_alloc(persist, 1, 1, &out1) == ErrorCode::Ok);
				const auto e = matrix_core::op_minor_matrix(one.view(), scratch, out1);
				assert(!matrix_core::is_ok(e));
				assert(e.code == ErrorCode::InvalidDimension);
		}

		{
				MatrixMutView out_bad;
				assert(matrix_core::matrix_alloc(persist, 3, 3, &out_bad) == ErrorCode::Ok);
				const auto e = matrix_core::op_minor_matrix(a.view(), scratch, out_bad);
				assert(!matrix_core::is_ok(e));
				assert(e.code == ErrorCode::DimensionMismatch);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_minor_matrix] after validation asserts\n");
#endif

		return 0;
}
