#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::keep_cursor_in_view;
using detail::kKbGroup1;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
using detail::op_enabled;
using detail::op_name;

template <typename T, std::size_t N> constexpr std::uint8_t array_count(const T (&)[N]) noexcept {
		static_assert(N <= 32);
		return static_cast<std::uint8_t>(N);
}

constexpr std::uint8_t kMenuMaxEntries = 16;

enum class MenuEntryKind : std::uint8_t {
		Submenu = 0,
		Operation,
};

struct MenuEntry {
		const char* label = nullptr;
		MenuEntryKind kind = MenuEntryKind::Submenu;
		MenuId submenu = MenuId::Main;     // when kind==Submenu
		OperationId op = OperationId::Add; // when kind==Operation
};

struct OpMenuEntry {
		OperationId op = OperationId::Add;
		const char* label = nullptr; // optional override; otherwise uses `op_name(op)`
};

constexpr MenuEntry kMainMenuEntries[] = {
        {"Matrices", MenuEntryKind::Submenu, MenuId::Matrices, OperationId::Add},
        {"Operations", MenuEntryKind::Submenu, MenuId::Operations, OperationId::Add},
};

constexpr MenuEntry kMatricesMenuEntries[] = {
        {"Edit Slots", MenuEntryKind::Submenu, MenuId::SlotList, OperationId::Add},
};

constexpr MenuEntry kOperationsMenuEntries[] = {
        {"Add/Subtract", MenuEntryKind::Submenu, MenuId::AddSub, OperationId::Add},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Mul},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Dot},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Cross},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Det},
        {"REF/RREF", MenuEntryKind::Submenu, MenuId::RefRref, OperationId::Add},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::SolveRref},
        {"Span/Independence", MenuEntryKind::Submenu, MenuId::Span, OperationId::Add},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Transpose},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Inverse},
        {"Spaces", MenuEntryKind::Submenu, MenuId::Spaces, OperationId::Add},
        {nullptr, MenuEntryKind::Operation, MenuId::Main, OperationId::Projection},
};

constexpr OpMenuEntry kAddSubMenuOps[] = {
        {OperationId::Add, nullptr},
        {OperationId::Sub, nullptr},
};

constexpr OpMenuEntry kRefRrefMenuOps[] = {
        {OperationId::Ref, nullptr},
        {OperationId::Rref, nullptr},
};

constexpr OpMenuEntry kSpanMenuOps[] = {
        {OperationId::SolveRref, "In span? (solve)"},
        {OperationId::SpanTest, nullptr},
        {OperationId::IndepTest, nullptr},
        {OperationId::ColSpaceBasis, "Basis from set"},
};

constexpr OpMenuEntry kSpacesMenuOps[] = {
        {OperationId::ColSpaceBasis, nullptr},
        {OperationId::RowSpaceBasis, nullptr},
        {OperationId::NullSpaceBasis, nullptr},
        {OperationId::LeftNullSpaceBasis, nullptr},
};

const char* menu_entry_label(const MenuEntry& e) noexcept {
		if (e.label)
				return e.label;
		if (e.kind == MenuEntryKind::Operation)
				return op_name(e.op);
		return "";
}

const char* op_menu_entry_label(const OpMenuEntry& e) noexcept {
		return e.label ? e.label : op_name(e.op);
}

std::uint8_t build_visible_menu(const MenuEntry* base, std::uint8_t base_count, const MenuEntry** out) noexcept {
		std::uint8_t count = 0;
		for (std::uint8_t i = 0; i < base_count; i++) {
				const MenuEntry& e = base[i];
				if (e.kind == MenuEntryKind::Operation && !op_enabled(e.op))
						continue;
				if (count >= kMenuMaxEntries)
						fail_fast("build_visible_menu overflow");
				out[count++] = &e;
		}
		return count;
}

std::uint8_t build_visible_op_menu(const OpMenuEntry* base, std::uint8_t base_count, const OpMenuEntry** out) noexcept {
		std::uint8_t count = 0;
		for (std::uint8_t i = 0; i < base_count; i++) {
				const OpMenuEntry& e = base[i];
				if (!op_enabled(e.op))
						continue;
				if (count >= kMenuMaxEntries)
						fail_fast("build_visible_op_menu overflow");
				out[count++] = &e;
		}
		return count;
}
} // namespace

void App::render_menu(const MenuState& s) noexcept {
		const ui::Layout l = ui::Layout{};

		const char* title = "Menu";
		const MenuEntry* base_entries = nullptr;
		std::uint8_t base_count = 0;
		const OpMenuEntry* base_ops = nullptr;
		std::uint8_t base_ops_count = 0;

		if (s.id == MenuId::Main) {
				title = "Main";
				base_entries = kMainMenuEntries;
				base_count = array_count(kMainMenuEntries);
				render_footer_hint("ENTER: Select  CLEAR: Exit");
		} else if (s.id == MenuId::Matrices) {
				title = "Matrices";
				base_entries = kMatricesMenuEntries;
				base_count = array_count(kMatricesMenuEntries);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		} else if (s.id == MenuId::SlotList) {
				title = "Edit Matrix";
				render_footer_hint("ENTER: Edit  2ND: Resize  DEL: Clear  CLEAR: Back");
		} else if (s.id == MenuId::Operations) {
				title = "Operations";
				base_entries = kOperationsMenuEntries;
				base_count = array_count(kOperationsMenuEntries);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		}
		else if (s.id == MenuId::Span) {
				title = "Span/Indep";
				base_ops = kSpanMenuOps;
				base_ops_count = array_count(kSpanMenuOps);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		} else if (s.id == MenuId::Spaces) {
				title = "Spaces";
				base_ops = kSpacesMenuOps;
				base_ops_count = array_count(kSpacesMenuOps);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		} else if (s.id == MenuId::AddSub) {
				title = "Add/Subtract";
				base_ops = kAddSubMenuOps;
				base_ops_count = array_count(kAddSubMenuOps);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		} else if (s.id == MenuId::RefRref) {
				title = "REF/RREF";
				base_ops = kRefRrefMenuOps;
				base_ops_count = array_count(kRefRrefMenuOps);
				render_footer_hint("ENTER: Select  CLEAR: Back");
		}

		render_header(title);

		const MenuEntry* visible_entries[kMenuMaxEntries]{};
		const OpMenuEntry* visible_ops[kMenuMaxEntries]{};
		std::uint8_t count = 0;
		if (s.id == MenuId::SlotList) {
				count = kSlotCount;
		} else if (base_entries) {
				count = build_visible_menu(base_entries, base_count, visible_entries);
		} else if (base_ops) {
				count = build_visible_op_menu(base_ops, base_ops_count, visible_ops);
		} else {
				fail_fast("Unhandled MenuId in render_menu");
		}

		const int list_top = l.header_h + l.margin_y;
		const int line_h = 14;
		const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;

		const std::uint8_t scroll = s.scroll;
		const std::uint8_t cursor = s.cursor;

		for (std::uint8_t row = 0; row < visible; row++) {
				const std::uint8_t idx = static_cast<std::uint8_t>(scroll + row);
				if (idx >= count)
						break;

				const int y = list_top + row * line_h;
				const bool selected = (idx == cursor);
				if (selected) {
						gfx_SetColor(ui::color::kBlue);
						gfx_FillRectangle(0, y - 1, kScreenW, line_h);
						gfx_SetTextFGColor(ui::color::kWhite);
				} else {
						gfx_SetTextFGColor(ui::color::kBlack);
				}

				gfx_SetTextXY(l.margin_x, y + 2);

				if (s.id == MenuId::SlotList) {
						const char name = static_cast<char>('A' + idx);
						gfx_PrintChar(name);
						gfx_PrintString(": ");

						const Slot& slot = slots_[idx];
						if (!slot.is_set()) {
								gfx_PrintString("(unset)");
						} else {
								gfx_PrintChar(static_cast<char>('0' + slot.rows));
								gfx_PrintChar('x');
								gfx_PrintChar(static_cast<char>('0' + slot.cols));
						}
				} else {
						if (base_entries)
								gfx_PrintString(menu_entry_label(*visible_entries[idx]));
						else if (base_ops)
								gfx_PrintString(op_menu_entry_label(*visible_ops[idx]));
				}
		}
}

void App::update_menu(MenuState& s) noexcept {
		const bool clear_pressed = input_.pressed(kKbGroup6, kb_Clear);
		if (clear_pressed) {
				SHELL_DBG("[menu] CLEAR back id=%u cursor=%u scroll=%u\n", (unsigned)s.id, (unsigned)s.cursor, (unsigned)s.scroll);
				REQUIRE(pop(), "pop failed");
				return;
		}

		const MenuEntry* base_entries = nullptr;
		std::uint8_t base_count = 0;
		const OpMenuEntry* base_ops = nullptr;
		std::uint8_t base_ops_count = 0;

		if (s.id == MenuId::Main) {
				base_entries = kMainMenuEntries;
				base_count = array_count(kMainMenuEntries);
		} else if (s.id == MenuId::Matrices) {
				base_entries = kMatricesMenuEntries;
				base_count = array_count(kMatricesMenuEntries);
		} else if (s.id == MenuId::Operations) {
				base_entries = kOperationsMenuEntries;
				base_count = array_count(kOperationsMenuEntries);
		}
		else if (s.id == MenuId::Span) {
				base_ops = kSpanMenuOps;
				base_ops_count = array_count(kSpanMenuOps);
		} else if (s.id == MenuId::Spaces) {
				base_ops = kSpacesMenuOps;
				base_ops_count = array_count(kSpacesMenuOps);
		} else if (s.id == MenuId::AddSub) {
				base_ops = kAddSubMenuOps;
				base_ops_count = array_count(kAddSubMenuOps);
		} else if (s.id == MenuId::RefRref) {
				base_ops = kRefRrefMenuOps;
				base_ops_count = array_count(kRefRrefMenuOps);
		}

		const MenuEntry* visible_entries[kMenuMaxEntries]{};
		const OpMenuEntry* visible_ops[kMenuMaxEntries]{};
		std::uint8_t count = 0;
		if (s.id == MenuId::SlotList) {
				count = kSlotCount;
		} else if (base_entries) {
				count = build_visible_menu(base_entries, base_count, visible_entries);
		} else if (base_ops) {
				count = build_visible_op_menu(base_ops, base_ops_count, visible_ops);
		} else {
				count = 0;
		}

		if (count == 0)
				fail_fast("Unhandled MenuId in update_menu count");

		if (s.cursor >= count)
				s.cursor = static_cast<std::uint8_t>(count - 1u);

		const std::uint8_t old_cursor = s.cursor;
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.cursor > 0)
						s.cursor--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.cursor + 1 < count)
						s.cursor++;
		}
		if (s.cursor != old_cursor) {
				const int line_h = 14;
				const ui::Layout l = ui::Layout{};
				const int visible = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / line_h;
				s.scroll = keep_cursor_in_view(s.cursor, s.scroll, count, visible);
		}

		if (s.id == MenuId::SlotList) {
				const std::uint8_t slot = s.cursor;
				const Slot& sl = slots_[slot];

				// resize: 2ND
				if (input_.pressed(kKbGroup1, kb_2nd)) {
						SHELL_DBG("[slotlist] 2ND resize sel=%c set=%u\n", (char)('A' + slot), (unsigned)sl.is_set());
						if (sl.is_set())
								REQUIRE(push(Page::make_confirm(slot, ConfirmAction::Resize)), "push confirm resize failed");
						else
								REQUIRE(push(Page::make_dim(slot, 2, 2)), "push dim failed");
						return;
				}

				// clear slot: DEL
				if (input_.pressed(kKbGroup1, kb_Del)) {
						SHELL_DBG("[slotlist] DEL clear sel=%c set=%u\n", (char)('A' + slot), (unsigned)sl.is_set());
						if (sl.is_set())
								REQUIRE(push(Page::make_confirm(slot, ConfirmAction::Clear)), "push confirm clear failed");
						else
								show_message("Slot is unset");
						return;
				}

				if (!input_.pressed(kKbGroup6, kb_Enter))
						return;

				SHELL_DBG("[slotlist] ENTER edit sel=%c set=%u rows=%u cols=%u\n",
				        (char)('A' + slot),
				        (unsigned)sl.is_set(),
				        (unsigned)sl.rows,
				        (unsigned)sl.cols);

				// edit entries: ENTER
				if (sl.is_set())
						REQUIRE(push(Page::make_editor(slot)), "push editor failed");
				else
						REQUIRE(push(Page::make_dim(slot, 2, 2)), "push dim failed");
				return;
		}

		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		if (base_entries) {
				const MenuEntry& e = *visible_entries[s.cursor];
				if (e.kind == MenuEntryKind::Submenu) {
						SHELL_DBG("[menu] submenu id=%u cursor=%u -> %u\n", (unsigned)s.id, (unsigned)s.cursor, (unsigned)e.submenu);
						REQUIRE(push(Page::make_menu(e.submenu)), "push submenu menu failed");
						return;
				}

				SHELL_DBG("[menu] op id=%u cursor=%u -> %u\n", (unsigned)s.id, (unsigned)s.cursor, (unsigned)e.op);
				REQUIRE(push(Page::make_slot_pick(e.op)), "push slot pick from menu failed");
				return;
		}

		if (base_ops) {
				const OperationId op = visible_ops[s.cursor]->op;
				SHELL_DBG("[menu] opmenu id=%u cursor=%u -> %u\n", (unsigned)s.id, (unsigned)s.cursor, (unsigned)op);
				REQUIRE(push(Page::make_slot_pick(op)), "push slot pick from op menu failed");
				return;
		}

		fail_fast("Unhandled MenuId in update_menu selection");
}

} // namespace matrix_shell
