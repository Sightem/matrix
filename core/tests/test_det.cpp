#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>

using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static Rational a_cells[4]{};
		MatrixMutView a{2, 2, 2, a_cells};
		const std::int64_t cases[][5] = {
		        {1, 2, 2, 4, 0},
		        {0, 1, 1, 0, -1},
		};
		for (const auto& values : cases) {
				for (std::uint8_t r = 0; r < 2; r++) {
						for (std::uint8_t c = 0; c < 2; c++)
								a.at_mut(r, c) = Rational::from_int(values[r * 2 + c]);
				}
				Rational det;
				assert(matrix_core::is_ok(matrix_core::op_det(a.view(), scratch, &det, nullptr)));
				assert(det.num() == values[4] && det.den() == 1);
		}

		static Rational ns_cells[6]{};
		MatrixMutView ns{2, 3, 3, ns_cells};
		Rational det;
		assert(matrix_core::op_det(ns.view(), scratch, &det, nullptr).code == ErrorCode::NotSquare);
		return 0;
}
