#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>

using matrix_core::ErrorCode;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::StepRenderBuffers;

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static Rational a_cells[16]{};
		MatrixMutView a{4, 4, 4, a_cells};
		const std::int64_t avals[4][4] = {
		        {-1, -4, 2, 1},
		        {2, -1, 7, 9},
		        {-1, 1, 3, 1},
		        {1, -2, 1, -4},
		};
		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++)
						a.at_mut(r, c) = Rational::from_int(avals[r][c]);
		}

		static Rational b_cells[4]{};
		MatrixMutView b{4, 1, 1, b_cells};
		const std::int64_t bvals[4] = {-32, 14, 11, -4};
		for (std::uint8_t r = 0; r < 4; r++)
				b.at_mut(r, 0) = Rational::from_int(bvals[r]);

		static Rational x_cells[4]{};
		MatrixMutView x{4, 1, 1, x_cells};
		assert(matrix_core::is_ok(matrix_core::op_cramer_solve(a.view(), b.view(), scratch, x)));

		Rational delta;
		assert(matrix_core::is_ok(matrix_core::op_det(a.view(), scratch, &delta, nullptr)));
		assert(delta.num() == -423 && delta.den() == 1);
		const std::int64_t expected_x[4] = {5, 8, 3, -1};
		const std::int64_t expected_delta_i[4] = {-2115, -3384, -1269, 423};
		Explanation expl;
		for (std::uint8_t col = 0; col < 4; col++) {
				Rational delta_i;
				assert(matrix_core::is_ok(matrix_core::op_det_replace_column(a.view(), b.view(), col, scratch, &delta_i, &expl)));
				assert(delta_i.num() == expected_delta_i[col] && delta_i.den() == 1);
		}
		static char latex[512];
		StepRenderBuffers bufs{nullptr, 0, latex, sizeof(latex), scratch};
		assert(expl.render_step(expl.step_count() - 1, bufs) == ErrorCode::Ok);
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++) {
						const auto& v = scratch.at(r, c);
						assert(v.num() == (c == 3 ? bvals[r] : avals[r][c]) && v.den() == 1);
				}
		}

		for (std::uint8_t r = 0; r < 4; r++) {
				assert(x.at(r, 0).num() == expected_x[r] && x.at(r, 0).den() == 1);
				assert(b.at(r, 0).num() == bvals[r] && b.at(r, 0).den() == 1);
				for (std::uint8_t c = 0; c < 4; c++)
						assert(a.at(r, c).num() == avals[r][c] && a.at(r, c).den() == 1);
		}

		MatrixMutView bad_b{1, 4, 4, b_cells};
		assert(matrix_core::op_cramer_solve(a.view(), bad_b.view(), scratch, x).code == ErrorCode::DimensionMismatch);
		return 0;
}
