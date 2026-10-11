#include "matrix_core/latex.hpp"
#include "matrix_core/writer.hpp"

#include <cassert>
#include <cstdint>

namespace matrix_core::latex {
namespace {

void write_rational_inner(const Rational& r, Writer& w) noexcept {
		w.append_rational_latex(r);
}

void write_rational_display_inner(const Rational& r, Writer& w) noexcept {
		w.append("$$");
		write_rational_inner(r, w);
		w.append("$$");
}

const char* begin_env(MatrixBrackets b) noexcept {
		switch (b) {
		case MatrixBrackets::BMatrix:
				return "\\begin{bmatrix}";
		case MatrixBrackets::PMatrix:
				return "\\begin{pmatrix}";
		case MatrixBrackets::VMatrix:
				return "\\begin{vmatrix}";
		}
		assert(false && "invalid MatrixBrackets");
		return nullptr;
}

const char* end_env(MatrixBrackets b) noexcept {
		switch (b) {
		case MatrixBrackets::BMatrix:
				return "\\end{bmatrix}";
		case MatrixBrackets::PMatrix:
				return "\\end{pmatrix}";
		case MatrixBrackets::VMatrix:
				return "\\end{vmatrix}";
		}
		assert(false && "invalid MatrixBrackets");
		return nullptr;
}

ErrorCode write_matrix_inner(MatrixView m, MatrixBrackets brackets, Writer& w) noexcept {
		if (m.rows != 0 && m.cols != 0 && !m.data)
				return ErrorCode::Internal;

		const char* begin = begin_env(brackets);
		const char* end = end_env(brackets);
		if (!begin || !end)
				return ErrorCode::Internal;

		w.append(begin);
		for (std::uint8_t row = 0; row < m.rows && m.cols != 0 && is_ok(w.status()); row++) {
				for (std::uint8_t col = 0; col < m.cols && is_ok(w.status()); col++) {
						if (col != 0)
								w.append(" & ");
						write_rational_inner(m.at(row, col), w);
				}
				if (row + 1 < m.rows)
						w.append(" \\\\ ");
		}
		w.append(end);
		return w.status();
}

ErrorCode write_matrix_display_inner(MatrixView m, MatrixBrackets brackets, Writer& w) noexcept {
		w.append("$$");
		ErrorCode ec = write_matrix_inner(m, brackets, w);
		if (!is_ok(ec))
				return ec;
		w.append("$$");
		return w.status();
}

ErrorCode write_augmented_matrix_inner(MatrixView left, MatrixView right, Writer& w) noexcept {
		if (!left.data || !right.data)
				return ErrorCode::Internal;
		if (left.rows != right.rows)
				return ErrorCode::DimensionMismatch;
		if (left.cols == 0 || right.cols == 0)
				return ErrorCode::InvalidDimension;

		w.append("\\left[\\begin{array}{");
		for (std::uint8_t i = 0; i < left.cols && is_ok(w.status()); i++)
				w.put('r');
		w.put('|');
		for (std::uint8_t i = 0; i < right.cols && is_ok(w.status()); i++)
				w.put('r');
		w.append("}");

		const std::uint8_t total_cols = static_cast<std::uint8_t>(left.cols + right.cols);
		for (std::uint8_t row = 0; row < left.rows && is_ok(w.status()); row++) {
				for (std::uint8_t col = 0; col < total_cols && is_ok(w.status()); col++) {
						if (col != 0)
								w.append(" & ");
						if (col < left.cols)
								write_rational_inner(left.at(row, col), w);
						else
								write_rational_inner(right.at(row, static_cast<std::uint8_t>(col - left.cols)), w);
				}
				if (row + 1 < left.rows)
						w.append(" \\\\ ");
		}
		w.append("\\end{array}\\right]");
		return w.status();
}

ErrorCode write_augmented_matrix_display_inner(MatrixView left, MatrixView right, Writer& w) noexcept {
		w.append("$$");
		ErrorCode ec = write_augmented_matrix_inner(left, right, w);
		if (!is_ok(ec))
				return ec;
		w.append("$$");
		return w.status();
}

} // namespace

ErrorCode write_rational(const Rational& r, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		write_rational_inner(r, w);
		return w.status();
}

ErrorCode write_rational_display(const Rational& r, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		write_rational_display_inner(r, w);
		return w.status();
}

ErrorCode write_matrix(MatrixView m, MatrixBrackets brackets, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		return write_matrix_inner(m, brackets, w);
}

ErrorCode write_matrix_display(MatrixView m, MatrixBrackets brackets, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		return write_matrix_display_inner(m, brackets, w);
}

ErrorCode write_augmented_matrix(MatrixView left, MatrixView right, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		return write_augmented_matrix_inner(left, right, w);
}

ErrorCode write_augmented_matrix_display(MatrixView left, MatrixView right, Buffer out) noexcept {
		Writer w{out.data, out.cap, 0};
		return write_augmented_matrix_display_inner(left, right, w);
}

} // namespace matrix_core::latex
