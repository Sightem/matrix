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

} // namespace matrix_shell::ui
