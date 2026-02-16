#pragma once

#include <cstdint>

#include "matrix_core/matrix.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/writer.hpp"

#if defined(MATRIX_CE_TESTS)
#include <debug.h>
#endif

namespace matrix_test_ce {

inline void print_str(const char* s) noexcept {
#if defined(MATRIX_CE_TESTS)
		dbg_printf("%s\n", s ? s : "(null)");
#else
		(void)s;
#endif
}

inline void print_i64(const char* label, std::int64_t v) noexcept {
#if defined(MATRIX_CE_TESTS)
		static char buf[128];
		matrix_core::Writer w{buf, sizeof(buf), 0};
		if (label) {
				(void)w.append(label);
				(void)w.append("=");
		}
		(void)w.append_i64(v);
		dbg_printf("%s\n", buf);
#else
		(void)label;
		(void)v;
#endif
}

inline void print_u64(const char* label, std::uint64_t v) noexcept {
#if defined(MATRIX_CE_TESTS)
		static char buf[128];
		matrix_core::Writer w{buf, sizeof(buf), 0};
		if (label) {
				(void)w.append(label);
				(void)w.append("=");
		}
		(void)w.append_u64(v);
		dbg_printf("%s\n", buf);
#else
		(void)label;
		(void)v;
#endif
}

inline void append_rational_ascii(matrix_core::Writer& w, const matrix_core::Rational& r) noexcept {
		(void)w.append_i64(r.num());
		if (r.den() != 1) {
				(void)w.put('/');
				(void)w.append_i64(r.den());
		}
}

inline void print_rational(const char* label, const matrix_core::Rational& r) noexcept {
#if defined(MATRIX_CE_TESTS)
		static char buf[128];
		matrix_core::Writer w{buf, sizeof(buf), 0};
		if (label) {
				(void)w.append(label);
				(void)w.append("=");
		}
		append_rational_ascii(w, r);
		dbg_printf("%s\n", buf);
#else
		(void)label;
		(void)r;
#endif
}

inline void print_matrix(const char* label, matrix_core::MatrixView m) noexcept {
#if defined(MATRIX_CE_TESTS)
		static char buf[256];
		{
				matrix_core::Writer w{buf, sizeof(buf), 0};
				if (label) {
						(void)w.append(label);
						(void)w.append(" ");
				}
				(void)w.append("(");
				(void)w.append_u64(m.rows);
				(void)w.put('x');
				(void)w.append_u64(m.cols);
				(void)w.append(")");
				dbg_printf("%s\n", buf);
		}

		for (std::uint8_t r = 0; r < m.rows; r++) {
				matrix_core::Writer w{buf, sizeof(buf), 0};
				(void)w.append("  [");
				for (std::uint8_t c = 0; c < m.cols; c++) {
						if (c != 0)
								(void)w.append(", ");
						append_rational_ascii(w, m.at(r, c));
				}
				(void)w.append("]");
				dbg_printf("%s\n", buf);
		}
#else
		(void)label;
		(void)m;
#endif
}

} // namespace matrix_test_ce
