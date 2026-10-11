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

void update_list(Input& input, std::uint8_t& cursor, std::uint8_t& scroll, std::uint8_t count) noexcept;
void render_slot(std::uint8_t index, const Slot& slot) noexcept;

bool op_is_binary(OperationId op) noexcept;
bool op_enabled(OperationId op) noexcept;
const char* op_name(OperationId op) noexcept;

void dbg_print_i64(std::int64_t v) noexcept;

} // namespace matrix_shell::detail
