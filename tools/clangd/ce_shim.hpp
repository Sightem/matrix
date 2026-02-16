#pragma once

#include <cstdint>

#if !defined(__INT24_TYPE__) && !defined(int24_t)
using int24_t = std::int32_t;
using uint24_t = std::uint32_t;
#endif
