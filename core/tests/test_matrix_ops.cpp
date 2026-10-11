#include "matrix_core/matrix_core.hpp"
#include "matrix_core/writer.hpp"

#include <cassert>
#include <cstring>

using matrix_core::ErrorCode;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;

int main() {
		{
				Rational cells[12]{};
				MatrixMutView work;
				assert(matrix_core::matrix_work_view({3, 3, 4, cells}, 2, 2, &work) == ErrorCode::Ok);
				assert(work.rows == 2 && work.cols == 2 && work.stride == 4);
				work.at_mut(1, 1) = Rational::from_int(7);
				assert(cells[5].num() == 7);
				assert(matrix_core::matrix_work_view({3, 3, 4, cells}, 4, 2, &work) == ErrorCode::Overflow);
				assert(work.rows == 2 && work.cols == 2 && work.data == cells);
				assert(matrix_core::matrix_work_view({}, 6, 0, &work) == ErrorCode::Ok);
				assert(work.rows == 6 && work.cols == 0 && work.data == nullptr);
		}
		{
				char bytes[5] = {'?', '?', '?', '?', '!'};
				matrix_core::Writer w(bytes, 4);
				w.append("abcd");
				assert(w.status() == ErrorCode::BufferTooSmall);
				assert(w.len == 3 && std::strcmp(bytes, "abc") == 0 && bytes[4] == '!');
				w.append_i64(-42);
				assert(w.status() == ErrorCode::BufferTooSmall);
				assert(w.len == 3 && std::strcmp(bytes, "abc") == 0 && bytes[4] == '!');
		}

		static Rational a_cells[4], b_cells[4], out_cells[4];
		MatrixMutView a{2, 2, 2, a_cells}, b{2, 2, 2, b_cells}, out{2, 2, 2, out_cells};
		for (std::uint8_t r = 0; r < 2; r++) {
				for (std::uint8_t c = 0; c < 2; c++) {
						a.at_mut(r, c) = Rational::from_int(1 + 2 * r + c);
						b.at_mut(r, c) = Rational::from_int(5 + 2 * r + c);
				}
		}

		Explanation expl;
		assert(matrix_core::is_ok(matrix_core::op_add(a.view(), b.view(), out, &expl)));
		for (std::uint8_t r = 0; r < 2; r++) {
				for (std::uint8_t c = 0; c < 2; c++)
						assert(out.at(r, c).num() == 6 + 4 * r + 2 * c && out.at(r, c).den() == 1);
		}

		const auto mismatch = matrix_core::op_add(a.view(), {1, 2, 2, b_cells}, out, &expl);
		assert(mismatch.code == ErrorCode::DimensionMismatch);
		assert(!expl.available());

		assert(matrix_core::is_ok(matrix_core::op_sub(a.view(), b.view(), out, nullptr)));
		for (const auto& cell : out_cells)
				assert(cell.num() == -4 && cell.den() == 1);

		assert(matrix_core::is_ok(matrix_core::op_mul(a.view(), b.view(), out, nullptr)));
		const std::int64_t product[4] = {19, 22, 43, 50};
		for (std::uint8_t i = 0; i < 4; i++)
				assert(out_cells[i].num() == product[i] && out_cells[i].den() == 1);
		assert(matrix_core::op_mul(a.view(), {1, 1, 1, b_cells}, out, nullptr).code == ErrorCode::DimensionMismatch);

		assert(matrix_core::is_ok(matrix_core::op_transpose(a.view(), out, nullptr)));
		const std::int64_t transposed[4] = {1, 3, 2, 4};
		for (std::uint8_t i = 0; i < 4; i++)
				assert(out_cells[i].num() == transposed[i] && out_cells[i].den() == 1);
		return 0;
}
