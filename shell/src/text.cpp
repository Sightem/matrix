#include "matrix_shell/text.hpp"

namespace matrix_shell {
namespace {

#if MATRIX_SHELL_LANG_FR
constexpr const char* kText[] = {
        "",
#define MATRIX_SHELL_TEXT_ENTRY(id, key, en, fr) fr,
#include "matrix_shell/text_catalog.inc"
#undef MATRIX_SHELL_TEXT_ENTRY
};
#else
constexpr const char* kText[] = {
        "",
#define MATRIX_SHELL_TEXT_ENTRY(id, key, en, fr) en,
#include "matrix_shell/text_catalog.inc"
#undef MATRIX_SHELL_TEXT_ENTRY
};
#endif

constexpr std::size_t kTextCount = static_cast<std::size_t>(TextId::Count);
static_assert((sizeof(kText) / sizeof(kText[0])) == kTextCount, "kText count mismatch");

} // namespace

const char* tr(TextId id) noexcept {
		const auto idx = static_cast<std::size_t>(id);
		if (idx >= kTextCount)
				return "";
		return kText[idx];
}

} // namespace matrix_shell
