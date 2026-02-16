#pragma once

#include "matrix_shell/app.hpp"
#include "matrix_shell/config.hpp"

#include <cstddef>
#include <cstdint>

#if MATRIX_SHELL_ENABLE_DEBUG
#include <debug.h>
#define SHELL_DBG(...) dbg_printf(__VA_ARGS__)
#else
#define SHELL_DBG(...) \
		do {            \
		} while (0)
#endif

namespace matrix_shell::detail {

constexpr int kScreenW = 320;
constexpr int kScreenH = 240;

constexpr std::uint8_t kKbGroupArrows = 7;
constexpr std::uint8_t kKbGroup1 = 1;
constexpr std::uint8_t kKbGroup6 = 6;
constexpr std::uint8_t kKbGroup3 = 3;
constexpr std::uint8_t kKbGroup4 = 4;
constexpr std::uint8_t kKbGroup5 = 5;

[[noreturn]] void fail_fast(const char* msg) noexcept;
[[noreturn]] void fail_fast(const char* func, const char* msg) noexcept;
void require_impl(bool ok, const char* func, const char* msg) noexcept;

std::uint8_t keep_cursor_in_view(std::uint8_t cursor, std::uint8_t scroll, std::uint8_t count, int visible) noexcept;

bool op_is_binary(OperationId op) noexcept;
bool op_enabled(OperationId op) noexcept;
const char* op_name(OperationId op) noexcept;

void dbg_print_i64(std::int64_t v) noexcept;
void dbg_print_rational(matrix_core::Rational r) noexcept;
void dbg_print_matrix(const char* tag, matrix_core::MatrixView m) noexcept;

} // namespace matrix_shell::detail

#define REQUIRE(ok, msg) ::matrix_shell::detail::require_impl((ok), __func__, (msg))
