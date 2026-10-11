#include "matrix_shell/detail/app_internal.hpp"
#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include <graphx.h>
#include <keypadc.h>

#include <cassert>
#include <cstdlib>

namespace matrix_shell::detail {


[[noreturn]] void fail_fast(const char* msg) noexcept {
		SHELL_DBG("[fatal] %s\n", msg);
		assert(false && "matrix_shell fail_fast");
		(void)msg;
		std::abort();
}


void update_list(Input& input, std::uint8_t& cursor, std::uint8_t& scroll, std::uint8_t count) noexcept {
		if (input.repeat(kKbGroupArrows, kb_Up) && cursor > 0)
				cursor--;
		if (input.repeat(kKbGroupArrows, kb_Down) && cursor + 1 < count)
				cursor++;
		if (cursor < scroll)
				scroll = cursor;
		if (cursor >= scroll + ui::kListVisible)
				scroll = static_cast<std::uint8_t>(cursor - ui::kListVisible + 1);
}

void render_slot(std::uint8_t index, const Slot& slot) noexcept {
		gfx_PrintChar(static_cast<char>('A' + index));
		gfx_PrintString(": ");
		if (!slot.is_set()) {
				gfx_PrintString(tr(TextId::CommonUnset));
		} else {
				gfx_PrintChar(static_cast<char>('0' + slot.rows));
				gfx_PrintChar('x');
				gfx_PrintChar(static_cast<char>('0' + slot.cols));
		}
}

namespace {
struct OpMeta {
		TextId name = TextId::None;
		bool binary = false;
		bool enabled = false;
};

constexpr OpMeta kOpMeta[] = {
        {TextId::OpAdd, true, true},
        {TextId::OpSub, true, true},
        {TextId::OpMul, true, true},
        {TextId::OpDot, true, true},
        {TextId::OpCross, true, true},
        /* Projection */
#if MATRIX_SHELL_ENABLE_PROJECTION && MATRIX_CORE_ENABLE_PROJECTION
        {TextId::OpProjection, true, true},
#else
        {TextId::None, false, false},
#endif
        /* Cramer */
#if MATRIX_SHELL_ENABLE_CRAMER && MATRIX_CORE_ENABLE_CRAMER
        {TextId::OpCramer, true, true},
#else
        {TextId::None, false, false},
#endif
        {TextId::OpDet, false, true},
        {TextId::OpSolveRref, true, true},
        {TextId::OpSpanTest, false, true},
        {TextId::OpIndepTest, false, true},
        {TextId::OpTranspose, false, true},
        {TextId::OpInverse, false, true},
        {TextId::OpColBasis, false, true},
        {TextId::OpRowBasis, false, true},
        {TextId::OpNullBasis, false, true},
        {TextId::OpLeftNullBasis, false, true},
        /* MinorMatrix */
#if MATRIX_SHELL_ENABLE_MINOR_MATRIX && MATRIX_CORE_ENABLE_MINOR_MATRIX
        {TextId::OpMinorMatrix, false, true},
#else
        {TextId::None, false, false},
#endif
        /* CofactorElement */
#if MATRIX_SHELL_ENABLE_COFACTOR && MATRIX_CORE_ENABLE_COFACTOR
        {TextId::OpCofactor, false, true},
#else
        {TextId::None, false, false},
#endif
        {TextId::OpRef, false, true},
        {TextId::OpRref, false, true},
};
static_assert(sizeof(kOpMeta) / sizeof(kOpMeta[0]) == static_cast<std::size_t>(OperationId::Rref) + 1u, "Update kOpMeta");

const OpMeta& op_meta(OperationId op) noexcept {
		const std::size_t idx = static_cast<std::size_t>(op);
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
		return meta.enabled ? tr(meta.name) : tr(TextId::CommonDisabled);
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

} // namespace matrix_shell::detail

namespace matrix_shell::ui {

void list_row(std::uint8_t row, bool selected) noexcept {
		const Layout l{};
		const int y = l.header_h + l.margin_y + row * kListLineHeight;
		if (selected) {
				gfx_SetColor(color::kBlue);
				gfx_FillRectangle(0, y - 1, detail::kScreenW, kListLineHeight);
		}
		gfx_SetTextFGColor(selected ? color::kWhite : color::kBlack);
		gfx_SetTextXY(l.margin_x, y + 2);
}

}
