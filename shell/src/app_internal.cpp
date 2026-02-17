#include "matrix_shell/detail/app_internal.hpp"
#include "matrix_shell/text.hpp"

#include <cassert>
#include <cstdlib>

namespace matrix_shell::detail {

using namespace matrix_shell::text_literals;

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
		TextId name = TextId::None;
		bool binary = false;
		bool enabled = false;
};

constexpr OpMeta kOpMeta[] = {
        /* Add */ {"op.add"_tid, true, true},
        /* Sub */ {"op.sub"_tid, true, true},
        /* Mul */ {"op.mul"_tid, true, true},
        /* Dot */ {"op.dot"_tid, true, true},
        /* Cross */ {"op.cross"_tid, true, true},
        /* Projection */
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
        {"op.projection"_tid, true, true},
#else
        {TextId::None, false, false},
#endif
        /* Cramer */
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
        {"op.cramer"_tid, true, true},
#else
        {TextId::None, false, false},
#endif
        /* Det */ {"op.det"_tid, false, true},
        /* SolveRref */ {"op.solve_rref"_tid, true, true},
        /* SpanTest */ {"op.span_test"_tid, false, true},
        /* IndepTest */ {"op.indep_test"_tid, false, true},
        /* Transpose */ {"op.transpose"_tid, false, true},
        /* Inverse */ {"op.inverse"_tid, false, true},
        /* ColSpaceBasis */ {"op.col_basis"_tid, false, true},
        /* RowSpaceBasis */ {"op.row_basis"_tid, false, true},
        /* NullSpaceBasis */ {"op.null_basis"_tid, false, true},
        /* LeftNullSpaceBasis */ {"op.left_null_basis"_tid, false, true},
        /* MinorMatrix */
#if MATRIX_SHELL_ENABLE_MINOR_MATRIX && MATRIX_CORE_ENABLE_MINOR_MATRIX
        {"op.minor_matrix"_tid, false, true},
#else
        {TextId::None, false, false},
#endif
        /* CofactorElement */
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
        {"op.cofactor"_tid, false, true},
#else
        {TextId::None, false, false},
#endif
        /* Ref */ {"op.ref"_tid, false, true},
        /* Rref */ {"op.rref"_tid, false, true},
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
		return op_meta(op).enabled;
}

const char* op_name(OperationId op) noexcept {
		const OpMeta& meta = op_meta(op);
		return meta.enabled ? tr(meta.name) : "common.disabled"_tx;
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
