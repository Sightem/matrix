#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>

using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static Rational a_cells[4] = {Rational::from_int(1), Rational::from_int(2),
		        Rational::from_int(3), Rational::from_int(4)};
		MatrixMutView a{2, 2, 2, a_cells};
		Rational cof;
		assert(matrix_core::is_ok(matrix_core::op_cofactor_element(a.view(), 0, 1, scratch, &cof, nullptr)));
		assert(cof.num() == -3 && cof.den() == 1);

		static Rational one_cells[1]{Rational::from_int(7)};
		MatrixMutView one{1, 1, 1, one_cells};
		assert(matrix_core::is_ok(matrix_core::op_cofactor_element(one.view(), 0, 0, scratch, &cof, nullptr)));
		assert(cof.num() == 1 && cof.den() == 1);

		assert(matrix_core::op_cofactor_element(a.view(), 2, 0, scratch, &cof, nullptr).code == ErrorCode::IndexOutOfRange);
		return 0;
}
