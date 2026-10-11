#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>

using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};

		static Rational a_cells[16]{};
		MatrixMutView a{4, 4, 4, a_cells};
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

		static Rational out_cells[16]{};
		MatrixMutView out{4, 4, 4, out_cells};
		const auto err = matrix_core::op_minor_matrix(a.view(), scratch, out);
		assert(matrix_core::is_ok(err));

		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++) {
						const Rational v = out.at(r, c);
						assert(v.den() == 1);
						assert(v.num() == expected_minor[r][c]);
				}
		}

		static Rational one_cells[1]{};
		static Rational out1_cells[1]{};
		MatrixMutView one{1, 1, 1, one_cells};
		MatrixMutView out1{1, 1, 1, out1_cells};
		assert(matrix_core::op_minor_matrix(one.view(), scratch, out1).code == ErrorCode::InvalidDimension);
		return 0;
}
