#include "matrix_shell/detail/app_internal.hpp"

#include <cassert>
#include <cstdlib>

namespace matrix_shell::detail {

[[noreturn]] void fail_fast(const char* msg) noexcept {
		assert(msg);
		SHELL_DBG("[fatal] %s\n", msg);
		assert(false && "matrix_shell fail_fast");
		(void)msg;
		std::abort();
}

[[noreturn]] void fail_fast(const char* func, const char* msg) noexcept {
		assert(func);
		assert(msg);
		SHELL_DBG("[fatal] %s: %s\n", func, msg);
		assert(false && "matrix_shell fail_fast");
		(void)func;
		(void)msg;
		std::abort();
}

void require_impl(bool ok, const char* func, const char* msg) noexcept {
		if (!ok)
				fail_fast(func, msg);
}

std::uint8_t keep_cursor_in_view(std::uint8_t cursor, std::uint8_t scroll, std::uint8_t count, int visible) noexcept {
		if (visible <= 0)
				fail_fast("keep_cursor_in_view: visible must be > 0");

		int scr = static_cast<int>(scroll);
		const int cur = static_cast<int>(cursor);

		if (cur < scr)
				scr = cur;

		const int last_visible = scr + visible - 1;
		if (cur > last_visible)
				scr = cur - (visible - 1);

		int max_scroll = static_cast<int>(count) - visible;
		if (max_scroll < 0)
				max_scroll = 0;
		if (scr < 0)
				scr = 0;
		if (scr > max_scroll)
				scr = max_scroll;

		return static_cast<std::uint8_t>(scr);
}

namespace {
struct OpMeta {
		const char* name = nullptr;
		bool binary = false;
};

// Note: name==nullptr implies the operation is disabled in this build.
// Keep this table small and constexpr for the CE build.
constexpr OpMeta kOpMeta[] = {
        /* Add */ {"Add", true},
        /* Sub */ {"Subtract", true},
        /* Mul */ {"Multiply", true},
        /* Dot */ {"Dot Product", true},
        /* Cross */ {"Cross Product", true},
        /* Projection */
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
        {"Projection", true},
#else
        {nullptr, false},
#endif
        /* Cramer */
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
        {"Cramer", true},
#else
        {nullptr, false},
#endif
        /* Det */ {"Determinant", false},
        /* SolveRref */ {"Solve (RREF)", true},
        /* SpanTest */ {"Spans R^m?", false},
        /* IndepTest */ {"Linearly Independent?", false},
        /* Transpose */ {"Transpose", false},
        /* Inverse */ {"Inverse", false},
        /* ColSpaceBasis */ {"Col(A) basis", false},
        /* RowSpaceBasis */ {"Row(A) basis", false},
        /* NullSpaceBasis */ {"Null(A) basis", false},
        /* LeftNullSpaceBasis */ {"Left Null(A) basis", false},
        /* MinorMatrix */
#if MATRIX_SHELL_ENABLE_MINOR_MATRIX && MATRIX_CORE_ENABLE_MINOR_MATRIX
        {"Minor Matrix", false},
#else
        {nullptr, false},
#endif
        /* CofactorElement */
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
        {"Cofactor", false},
#else
        {nullptr, false},
#endif
        /* Ref */ {"REF", false},
        /* Rref */ {"RREF", false},
};
static_assert(sizeof(kOpMeta) / sizeof(kOpMeta[0]) == static_cast<std::size_t>(OperationId::Rref) + 1u, "Update kOpMeta");

const OpMeta& op_meta(OperationId op) noexcept {
		const std::size_t idx = static_cast<std::size_t>(op);
		if (idx >= (sizeof(kOpMeta) / sizeof(kOpMeta[0])))
				fail_fast("Unhandled OperationId in op_meta");
		return kOpMeta[idx];
}
} // namespace

bool op_is_binary(OperationId op) noexcept {
		return op_meta(op).binary;
}

bool op_enabled(OperationId op) noexcept {
		return op_meta(op).name != nullptr;
}

const char* op_name(OperationId op) noexcept {
		const char* name = op_meta(op).name;
		return name ? name : "Disabled";
}

void dbg_print_i64(std::int64_t v) noexcept {
#if MATRIX_SHELL_ENABLE_DEBUG
		static char buf[32];
		char* p = buf + sizeof(buf) - 1;
		*p = '\0';

		const bool neg = v < 0;
		std::uint64_t mag = neg ? (static_cast<std::uint64_t>(-(v + 1)) + 1u) : static_cast<std::uint64_t>(v);
		do {
				*--p = static_cast<char>('0' + (mag % 10u));
				mag /= 10u;
		} while (mag != 0u);
		if (neg)
				*--p = '-';
		dbg_printf("%s", p);
#else
		(void)v;
#endif
}

void dbg_print_rational(matrix_core::Rational r) noexcept {
#if MATRIX_SHELL_ENABLE_DEBUG
		dbg_print_i64(r.num());
		dbg_printf("/");
		dbg_print_i64(r.den());
#else
		(void)r;
#endif
}

void dbg_print_matrix(const char* tag, matrix_core::MatrixView m) noexcept {
#if MATRIX_SHELL_ENABLE_DEBUG
		if (!tag)
				tag = "(null)";
		SHELL_DBG("[%s] %ux%u stride=%u data=%p\n", tag, m.rows, m.cols, m.stride, (const void*)m.data);
		if (!m.data)
				return;

		for (std::uint8_t r = 0; r < m.rows; r++) {
				for (std::uint8_t c = 0; c < m.cols; c++) {
						const matrix_core::Rational& v = m.at(r, c);
						dbg_printf("  (%u,%u)=", (unsigned)r, (unsigned)c);
						dbg_print_i64(v.num());
						dbg_printf("/");
						dbg_print_i64(v.den());
						dbg_printf("\n");
				}
		}
#else
		(void)tag;
		(void)m;
#endif
}

} // namespace matrix_shell::detail
