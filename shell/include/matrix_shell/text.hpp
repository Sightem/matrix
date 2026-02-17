#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace matrix_shell {

enum class TextId : std::uint16_t {
		None = 0,
#define MATRIX_SHELL_TEXT_ENTRY(id, key, en, fr) id,
#include "matrix_shell/text_catalog.inc"
#undef MATRIX_SHELL_TEXT_ENTRY
		Count
};

const char* tr(TextId id) noexcept;

namespace text_detail {

template <std::size_t N> struct FixedString {
		char value[N]{};
		static constexpr std::size_t size = N;

		consteval FixedString(const char (&str)[N]) noexcept {
				for (std::size_t i = 0; i < N; i++)
						value[i] = str[i];
		}
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

template <FixedString A, FixedString B> consteval bool fixed_string_equal() noexcept {
		if constexpr (A.size != B.size) {
				return false;
		} else {
				for (std::size_t i = 0; i < A.size; i++) {
						if (A.value[i] != B.value[i])
								return false;
				}
				return true;
		}
}

template <FixedString>
struct dependent_false : std::false_type {};

template <FixedString Key> consteval TextId key_to_id() noexcept {
#define MATRIX_SHELL_TEXT_ENTRY(id, key, en, fr)                                                                         \
		if constexpr (fixed_string_equal<Key, FixedString{key}>()) {                                                     \
				return TextId::id;                                                                                        \
		} else
#include "matrix_shell/text_catalog.inc"
#undef MATRIX_SHELL_TEXT_ENTRY
		{
				static_assert(dependent_false<Key>::value, "Unknown localization key used with _tx/_tid literal");
				return TextId::None;
		}
}

} // namespace text_detail

namespace text_literals {

template <text_detail::FixedString Key> consteval TextId operator""_tid() noexcept {
		return text_detail::key_to_id<Key>();
}

template <text_detail::FixedString Key> inline const char* operator""_tx() noexcept {
		return tr(text_detail::key_to_id<Key>());
}

} // namespace text_literals

} // namespace matrix_shell
