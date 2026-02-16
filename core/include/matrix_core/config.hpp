#pragma once

#include <cstddef>
#include <cstdint>

namespace matrix_core {
constexpr std::uint8_t kMaxRows = 6;
// Note: user-editable matrices are limited to 6 columns in the shell UI, but
// some internal computations (e.g. solve via augmented [A|b]) need 7.
constexpr std::uint8_t kMaxCols = 7;
constexpr std::size_t kMaxEntries = static_cast<std::size_t>(kMaxRows) * static_cast<std::size_t>(kMaxCols);
} // namespace matrix_core

#ifndef MATRIX_CORE_ENABLE_PROJECTION
#define MATRIX_CORE_ENABLE_PROJECTION 1
#endif
#ifndef MATRIX_CORE_ENABLE_MINOR_MATRIX
#define MATRIX_CORE_ENABLE_MINOR_MATRIX 1
#endif
#ifndef MATRIX_CORE_ENABLE_COFACTOR
#define MATRIX_CORE_ENABLE_COFACTOR 1
#endif
#ifndef MATRIX_CORE_ENABLE_CRAMER
#define MATRIX_CORE_ENABLE_CRAMER 1
#endif
