#include "matrix_core/matrix_core.hpp"

#include <cassert>

using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::ProjDecomposeResult;
using matrix_core::Rational;

int main() {
		static Rational u_cells[3] = {Rational::from_int(1), Rational::from_int(2), Rational::from_int(3)};
		static Rational v_cells[3] = {Rational::from_int(4), Rational::from_int(5), Rational::from_int(6)};
		MatrixMutView u{3, 1, 1, u_cells};
		MatrixMutView v{1, 3, 3, v_cells};
		Rational dot;
		assert(matrix_core::is_ok(matrix_core::op_dot(u.view(), v.view(), &dot, nullptr)));
		assert(dot.num() == 32 && dot.den() == 1);

		static Rational cross_cells[3]{};
		for (unsigned orientation = 0; orientation < 2; orientation++) {
				u = orientation == 0 ? MatrixMutView{3, 1, 1, u_cells} : MatrixMutView{1, 3, 3, u_cells};
				v = orientation == 0 ? MatrixMutView{1, 3, 3, v_cells} : MatrixMutView{3, 1, 1, v_cells};
				MatrixMutView cross{u.rows, u.cols, u.stride, cross_cells};
				assert(matrix_core::is_ok(matrix_core::op_cross(u.view(), v.view(), cross, nullptr)));
				assert(cross_cells[0].num() == -3 && cross_cells[0].den() == 1);
				assert(cross_cells[1].num() == 6 && cross_cells[1].den() == 1);
				assert(cross_cells[2].num() == -3 && cross_cells[2].den() == 1);
		}

#if MATRIX_CORE_ENABLE_PROJECTION
		static Rational proj_cells[3]{};
		static Rational orth_cells[3]{};
		MatrixMutView proj{1, 3, 3, proj_cells};
		MatrixMutView orth{1, 3, 3, orth_cells};
		ProjDecomposeResult result;
		const auto err = matrix_core::op_proj_decompose_u_onto_v(u.view(), v.view(), proj, orth, &result, nullptr);
		assert(matrix_core::is_ok(err));
		assert(result.k.num() == 32 && result.k.den() == 77);
		assert(proj.at(0, 0).num() == 128 && proj.at(0, 0).den() == 77);
		assert(proj.at(0, 1).num() == 160 && proj.at(0, 1).den() == 77);
		assert(proj.at(0, 2).num() == 192 && proj.at(0, 2).den() == 77);
		assert(orth.at(0, 0).num() == -51 && orth.at(0, 0).den() == 77);
		assert(orth.at(0, 1).num() == -6 && orth.at(0, 1).den() == 77);
		assert(orth.at(0, 2).num() == 39 && orth.at(0, 2).den() == 77);
		assert(result.proj_norm2.num() == 1024 && result.proj_norm2.den() == 77);
		assert(result.orth_norm2.num() == 54 && result.orth_norm2.den() == 77);
		matrix_core::matrix_fill_zero(v);
		assert(matrix_core::op_proj_decompose_u_onto_v(u.view(), v.view(), proj, orth, &result, nullptr).code == ErrorCode::DivisionByZero);
#endif
		return 0;
}
