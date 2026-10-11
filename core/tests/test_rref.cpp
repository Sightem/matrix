#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstring>

using matrix_core::EchelonKind;
using matrix_core::ErrorCode;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::StepRenderBuffers;

int main() {
		static Rational a_cells[4]{};
		MatrixMutView a{2, 2, 2, a_cells};
		a.at_mut(0, 0) = Rational::from_int(1);
		a.at_mut(0, 1) = Rational::from_int(2);
		a.at_mut(1, 0) = Rational::from_int(3);
		a.at_mut(1, 1) = Rational::from_int(4);
		static Rational out_cells[4]{};
		MatrixMutView out{2, 2, 2, out_cells};

		assert(matrix_core::is_ok(matrix_core::op_echelon(a.view(), EchelonKind::Ref, out, nullptr)));
		assert(out.at(0, 0).num() == 1 && out.at(0, 0).den() == 1);
		assert(out.at(0, 1).num() == 2 && out.at(0, 1).den() == 1);
		assert(out.at(1, 0).is_zero());
		assert(out.at(1, 1).num() == 2 && out.at(1, 1).den() == 1);

		Explanation expl;
		assert(matrix_core::is_ok(matrix_core::op_echelon(a.view(), EchelonKind::Rref, out, &expl)));
		assert(out.at(0, 0).num() == 1 && out.at(0, 0).den() == 1);
		assert(out.at(0, 1).is_zero());
		assert(out.at(1, 0).is_zero());
		assert(out.at(1, 1).num() == 1 && out.at(1, 1).den() == 1);

		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static char initial[512];
		static char rendered[512];
		StepRenderBuffers bufs{nullptr, 0, initial, sizeof(initial), scratch};
		const std::size_t last = expl.step_count() - 1;
		assert(expl.render_step(last, bufs) == ErrorCode::Ok);
		bufs.latex = rendered;
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		for (auto& cell : work_cells)
				cell = Rational::from_int(99);
		assert(expl.render_step(last, bufs) == ErrorCode::Ok);
		assert(std::strcmp(initial, rendered) == 0);
		for (std::uint8_t r = 0; r < 2; r++)
				for (std::uint8_t c = 0; c < 2; c++) {
						assert(scratch.at(r, c).num() == out.at(r, c).num());
						assert(scratch.at(r, c).den() == out.at(r, c).den());
				}
		assert(expl.render_step(expl.step_count(), bufs) == ErrorCode::StepOutOfRange);

		return 0;
}
