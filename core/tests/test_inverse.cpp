#include "matrix_core/matrix_core.hpp"

#include <cassert>

using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static Rational a_cells[4]{};
		MatrixMutView a{2, 2, 2, a_cells};
		a.at_mut(0, 0) = Rational::from_int(1);
		a.at_mut(0, 1) = Rational::from_int(2);
		a.at_mut(1, 0) = Rational::from_int(3);
		a.at_mut(1, 1) = Rational::from_int(4);
		static Rational inv_cells[4]{};
		MatrixMutView inv{2, 2, 2, inv_cells};
		assert(matrix_core::is_ok(matrix_core::op_inverse(a.view(), scratch, inv, nullptr)));
		assert(inv.at(0, 0).num() == -2 && inv.at(0, 0).den() == 1);
		assert(inv.at(0, 1).num() == 1 && inv.at(0, 1).den() == 1);
		assert(inv.at(1, 0).num() == 3 && inv.at(1, 0).den() == 2);
		assert(inv.at(1, 1).num() == -1 && inv.at(1, 1).den() == 2);

		a.at_mut(1, 0) = Rational::from_int(2);
		assert(matrix_core::op_inverse(a.view(), scratch, inv, nullptr).code == ErrorCode::Singular);

		static Rational boundary_cells[36]{};
		static Rational boundary_inv_cells[36]{};
		MatrixMutView boundary{6, 6, 6, boundary_cells};
		MatrixMutView boundary_inv{6, 6, 6, boundary_inv_cells};
		for (std::uint8_t r = 0; r < 6; r++)
				boundary.at_mut(r, 5 - r) = Rational::from_int(2);
		for (auto& cell : work_cells)
				cell = Rational::from_int(99);
		assert(matrix_core::is_ok(matrix_core::op_inverse(boundary.view(), scratch, boundary_inv, nullptr)));
		for (std::uint8_t r = 0; r < 6; r++)
				for (std::uint8_t c = 0; c < 6; c++) {
						assert(boundary_inv.at(r, c).num() == (c == 5 - r ? 1 : 0));
						assert(boundary_inv.at(r, c).den() == (c == 5 - r ? 2 : 1));
				}

		assert(matrix_core::op_inverse(boundary.view(), {6, 11, 12, work_cells}, boundary_inv, nullptr).code == ErrorCode::Overflow);
		return 0;
}
