#pragma once

#include <cstdint>

namespace matrix_shell::ui {

namespace color {
constexpr std::uint8_t kBlack = 0;
constexpr std::uint8_t kWhite = 1;
constexpr std::uint8_t kLightGray = 2;
constexpr std::uint8_t kDarkGray = 3;
constexpr std::uint8_t kBlue = 4;
} // namespace color

struct Layout {
		int header_h = 18;
		int footer_h = 20;
		int margin_x = 8;
		int margin_y = 6;
};

constexpr int kListLineHeight = 14;
constexpr int kListVisible = (240 - Layout{}.header_h - Layout{}.footer_h - 2 * Layout{}.margin_y) / kListLineHeight;

void list_row(std::uint8_t row, bool selected) noexcept;

} // namespace matrix_shell::ui
