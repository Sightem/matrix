#include "matrix_core/ops.hpp"

#if MATRIX_CORE_ENABLE_COFACTOR
#include "matrix_core/det_detail.hpp"
#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/writer.hpp"

namespace matrix_core {
namespace {
ErrorCode build_submatrix(MatrixView a, std::uint8_t del_r, std::uint8_t del_c, MatrixMutView storage, MatrixMutView* out) noexcept {
		MatrixMutView sub;
		ErrorCode ec = matrix_work_view(storage, static_cast<std::uint8_t>(a.rows - 1), static_cast<std::uint8_t>(a.cols - 1), &sub);
		if (!is_ok(ec))
				return ec;
		std::uint8_t dest_row = 0;
		for (std::uint8_t row = 0; row < a.rows; row++) {
				if (row == del_r)
						continue;
				std::uint8_t dest_col = 0;
				for (std::uint8_t col = 0; col < a.cols; col++) {
						if (col == del_c)
								continue;
						sub.at_mut(dest_row, dest_col++) = a.at(row, col);
				}
				dest_row++;
		}
		*out = sub;
		return ErrorCode::Ok;
}

ErrorCode write_cofactor_latex(const Explanation& e, char* out, std::size_t cap) noexcept {
		Writer w{out, cap};
		w.append("$$C_{");
		w.append_index1(e.row);
		w.append(",");
		w.append_index1(e.col);
		w.append("} = (-1)^{");
		w.append_u64(static_cast<std::uint32_t>(e.row) + e.col + 2u);
		w.append("} M_{");
		w.append_index1(e.row);
		w.append(",");
		w.append_index1(e.col);
		w.append("} = ");
		w.append_rational_latex(e.value);
		w.append("$$");
		return w.status();
}
} // namespace

ErrorCode Explanation::render_cofactor(std::size_t index, const StepRenderBuffers& out) const noexcept {
		MatrixMutView sub;
		ErrorCode ec = matrix_work_view(out.work, static_cast<std::uint8_t>(input.rows - 1),
		        static_cast<std::uint8_t>(input.cols - 1), &sub);
		if (!is_ok(ec))
				return ec;
		if (index == 0)
				return latex::write_matrix_display(input, latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
		if (index == step_count() - 1)
				return write_cofactor_latex(*this, out.latex, out.latex_cap);
		ec = build_submatrix(input, row, col, out.work, &sub);
		if (!is_ok(ec))
				return ec;
		if (index == 1) {
				if (out.caption && out.caption_cap) {
						Writer w{out.caption, out.caption_cap};
						w.append("Delete row ");
						w.append_index1(row);
						w.append(", col ");
						w.append_index1(col);
						if (!is_ok(w.status()))
								return w.status();
				}
		} else {
				const std::size_t stop_after = index - 1;
				Rational ignored;
				RowOp last{};
				std::size_t ops = 0;
				ec = detail::det_elim(sub, &ignored, &ops, stop_after, &last);
				if (!is_ok(ec))
						return ec;
				if (ops < stop_after)
						return ErrorCode::StepOutOfRange;
				if (out.caption) {
						ec = row_op_caption(last, out.caption, out.caption_cap);
						if (!is_ok(ec))
								return ec;
				}
		}
		return latex::write_matrix_display(sub.view(), latex::MatrixBrackets::BMatrix, {out.latex, out.latex_cap});
}

Error op_cofactor_element(MatrixView a, std::uint8_t i, std::uint8_t j, MatrixMutView storage,
        Rational* out, Explanation* expl) noexcept {
		if (expl)
				*expl = {};
		if (a.rows != a.cols)
				return err_not_square(a.dim());
		if (i >= a.rows || j >= a.cols) {
				Error err{ErrorCode::IndexOutOfRange, a.dim()};
				err.i = i;
				err.j = j;
				return err;
		}
		if (!out || !a.data)
				return {ErrorCode::Internal};
		if (a.stride < a.cols)
				return err_invalid_dim(a.dim());
		Rational minor = Rational::from_int(1);
		std::size_t op_count = 0;
		if (a.rows > 1) {
				MatrixMutView sub;
				ErrorCode ec = build_submatrix(a, i, j, storage, &sub);
				if (!is_ok(ec))
						return {ec, a.dim()};
				ec = detail::det_elim(sub, &minor, &op_count, static_cast<std::size_t>(-1), nullptr);
				if (!is_ok(ec))
						return {ec, a.dim()};
		}
		Rational cofactor = minor;
		if (((i + j) & 1u) != 0u) {
				ErrorCode ec = rational_neg(minor, &cofactor);
				if (!is_ok(ec))
						return {ec, a.dim()};
		}
		*out = cofactor;
		if (expl) {
				expl->kind = Explanation::Kind::Cofactor;
				expl->input = a;
				expl->row = i;
				expl->col = j;
				expl->value = cofactor;
				expl->op_count = op_count;
		}
		return {};
}
} // namespace matrix_core
#endif
