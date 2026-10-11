#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstring>

using matrix_core::EchelonKind;
using matrix_core::ErrorCode;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::SpaceInfo;

static MatrixMutView mat2(Rational* cells, std::int64_t a00, std::int64_t a01, std::int64_t a10, std::int64_t a11) {
		MatrixMutView m{2, 2, 2, cells};
		m.at_mut(0, 0) = Rational::from_int(a00);
		m.at_mut(0, 1) = Rational::from_int(a01);
		m.at_mut(1, 0) = Rational::from_int(a10);
		m.at_mut(1, 1) = Rational::from_int(a11);
		return m;
}

int main() {
		static Rational work_cells[72]{};
		MatrixMutView scratch{6, 12, 12, work_cells};
		static Rational saved_cells[42]{};
		static Rational result_cells[42]{};
		MatrixMutView saved{6, 7, 7, saved_cells};
		MatrixMutView particular{6, 1, 1, result_cells};
		MatrixMutView basis_storage{6, 6, 6, result_cells + 6};

		{
				static Rational a_cells[4]{};
				MatrixMutView a = mat2(a_cells, 0, 2, 0, 4);

				MatrixMutView rref;
				static Rational rref_cells_2[4]{};
				rref = {2, 2, 2, rref_cells_2};
				auto err = matrix_core::op_echelon(a.view(), EchelonKind::Rref, rref, nullptr);
				assert(matrix_core::is_ok(err));

				SpaceInfo info{};
				assert(matrix_core::space_info_from_rref(rref.view(), 2, &info) == ErrorCode::Ok);
				assert(info.rank == 1);
				assert(info.nullity == 1);
				assert(info.pivot_cols[0] == 1);

				MatrixMutView nbas{};
				static Rational null_cells[4]{};
				assert(matrix_core::space_null_basis(rref.view(), 2, info, {2, 2, 2, null_cells}, &nbas) == ErrorCode::Ok);
				assert(nbas.rows == 2 && nbas.cols == 1);
				assert(nbas.at(0, 0).num() == 1 && nbas.at(0, 0).den() == 1);
				assert(nbas.at(1, 0).is_zero());

				MatrixMutView rbas{};
				static Rational row_cells[4]{};
				assert(matrix_core::space_row_basis(rref.view(), 2, info, {2, 2, 2, row_cells}, &rbas) == ErrorCode::Ok);
				assert(rbas.rows == 1 && rbas.cols == 2);
				assert(rbas.at(0, 0).is_zero());
				assert(rbas.at(0, 1).num() == 1 && rbas.at(0, 1).den() == 1);

				MatrixMutView cbas{};
				static Rational col_cells[4]{};
				assert(matrix_core::space_col_basis(a.view(), info, {2, 2, 2, col_cells}, &cbas) == ErrorCode::Ok);
				assert(cbas.rows == 2 && cbas.cols == 1);
				assert(cbas.at(0, 0).num() == 2 && cbas.at(0, 0).den() == 1);
				assert(cbas.at(1, 0).num() == 4 && cbas.at(1, 0).den() == 1);
		}

		{
				MatrixMutView a;
				MatrixMutView b;
				static Rational a_cells_5[36]{};
				a = {6, 6, 6, a_cells_5};
				static Rational b_cells_6[6]{};
				b = {6, 1, 1, b_cells_6};
				for (std::uint8_t i = 0; i < 6; i++) {
						a.at_mut(i, i) = Rational::from_int(2);
						b.at_mut(i, 0) = Rational::from_int(i + 1);
				}
				matrix_core::SolveResult result;
				assert(matrix_core::is_ok(matrix_core::op_solve_rref(a.view(), b.view(), {}, scratch, particular, basis_storage, &result, nullptr)));
				assert(result.info.rank == 6 && result.info.nullity == 0);
				assert(result.particular.rows == 6 && result.particular.cols == 1);
				assert(result.null_basis.rows == 6 && result.null_basis.cols == 0 && result.null_basis.data == nullptr);
				for (std::uint8_t r = 0; r < 6; r++) {
						assert(result.particular.at(r, 0).num() == (r % 2 == 0 ? r + 1 : (r + 1) / 2));
						assert(result.particular.at(r, 0).den() == (r % 2 == 0 ? 2 : 1));
				}
		}

		{
				static Rational a_cells[4]{};
				MatrixMutView a = mat2(a_cells, 1, 1, 2, 2);
				MatrixMutView b;
				static Rational b_cells_7[2]{};
				b = {2, 1, 1, b_cells_7};
				b.at_mut(0, 0) = Rational::from_int(3);
				b.at_mut(1, 0) = Rational::from_int(6);
				matrix_core::SolveResult result;
				matrix_core::Explanation expl;
				assert(matrix_core::is_ok(matrix_core::op_solve_rref(a.view(), b.view(), saved, scratch, particular, basis_storage, &result, &expl)));
				static char initial[512];
				static char rendered[512];
				matrix_core::StepRenderBuffers buffers{nullptr, 0, initial, sizeof(initial), scratch};
				assert(expl.render_step(0, buffers) == ErrorCode::Ok);
				matrix_core::matrix_fill_zero(a);
				matrix_core::matrix_fill_zero(b);
				matrix_core::matrix_fill_zero(scratch);
				buffers.latex = rendered;
				assert(expl.render_step(expl.step_count() - 1, buffers) == ErrorCode::Ok);
				assert(expl.render_step(0, buffers) == ErrorCode::Ok);
				assert(std::strcmp(initial, rendered) == 0);
				assert(result.info.rank == 1 && result.info.nullity == 1);
				assert(result.particular.at(0, 0).num() == 3 && result.particular.at(0, 0).den() == 1);
				assert(result.particular.at(1, 0).is_zero());
				assert(result.null_basis.rows == 2 && result.null_basis.cols == 1);
				assert(result.null_basis.at(0, 0).num() == -1 && result.null_basis.at(0, 0).den() == 1);
				assert(result.null_basis.at(1, 0).num() == 1 && result.null_basis.at(1, 0).den() == 1);

				static Rational a_cells_8[36]{};
				a = {6, 6, 6, a_cells_8};
				static Rational b_cells_9[6]{};
				b = {6, 1, 1, b_cells_9};
				for (auto& value : result_cells)
						value = Rational::from_int(99);
				assert(matrix_core::is_ok(matrix_core::op_solve_rref(a.view(), b.view(), {}, scratch, particular, basis_storage, &result, nullptr)));
				assert(result.info.rank == 0 && result.info.nullity == 6);
				assert(result.particular.rows == 6 && result.particular.cols == 1);
				assert(result.null_basis.rows == 6 && result.null_basis.cols == 6);
				for (std::uint8_t r = 0; r < 6; r++) {
						assert(result.particular.at(r, 0).is_zero());
						for (std::uint8_t c = 0; c < 6; c++) {
								assert(result.null_basis.at(r, c).num() == (r == c ? 1 : 0));
								assert(result.null_basis.at(r, c).den() == 1);
						}
				}
				MatrixMutView basis;
				assert(matrix_core::space_col_basis(a.view(), result.info, {}, &basis) == ErrorCode::Ok);
				assert(basis.rows == 6 && basis.cols == 0 && basis.data == nullptr);
				assert(matrix_core::space_row_basis(a.view(), 6, result.info, {}, &basis) == ErrorCode::Ok);
				assert(basis.rows == 0 && basis.cols == 6 && basis.data == nullptr);
				b.at_mut(5, 0) = Rational::from_int(1);
				assert(matrix_core::op_solve_rref(a.view(), b.view(), {}, scratch, particular, basis_storage, &result, nullptr).code == ErrorCode::NoSolution);
		}

		return 0;
}
