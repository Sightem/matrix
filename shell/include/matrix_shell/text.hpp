#pragma once

#include <cstdint>

namespace matrix_shell {

enum class TextId : std::uint16_t {
		None = 0,
#define MATRIX_SHELL_TEXT_ENTRY(id, en, fr) id,
#include "matrix_shell/text_catalog.inc"
#undef MATRIX_SHELL_TEXT_ENTRY
		Count
};

const char* tr(TextId id) noexcept;

} // namespace matrix_shell
