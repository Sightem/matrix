#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"
#include "matrix_shell/detail/app_internal.hpp"

#include <algorithm>
#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::kKbGroup1;
using detail::kKbGroup6;

template <typename T, std::size_t N> constexpr std::uint8_t array_count(const T (&)[N]) noexcept {
		static_assert(N <= UINT8_MAX);
		return static_cast<std::uint8_t>(N);
}

enum class MenuEntryKind : std::uint8_t {
		Submenu,
		Operation,
};

struct MenuEntry {
		TextId label;
		MenuEntryKind kind;
		MenuId submenu;
		OperationId op;
};

constexpr MenuEntry kMainMenuEntries[] = {
        {TextId::MenuEntryMatrices, MenuEntryKind::Submenu, MenuId::Matrices, OperationId::Add},
        {TextId::MenuEntryOperations, MenuEntryKind::Submenu, MenuId::Operations, OperationId::Add},
};
constexpr MenuEntry kMatricesMenuEntries[] = {
        {TextId::MenuEntryEditSlots, MenuEntryKind::Submenu, MenuId::SlotList, OperationId::Add},
};
constexpr MenuEntry kOperationsMenuEntries[] = {
        {TextId::MenuEntryAddSub, MenuEntryKind::Submenu, MenuId::AddSub, OperationId::Add},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Mul},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Dot},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Cross},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Det},
        {TextId::MenuEntryRefRref, MenuEntryKind::Submenu, MenuId::RefRref, OperationId::Add},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::SolveRref},
        {TextId::MenuEntrySpanIndep, MenuEntryKind::Submenu, MenuId::Span, OperationId::Add},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Transpose},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Inverse},
        {TextId::MenuEntrySpaces, MenuEntryKind::Submenu, MenuId::Spaces, OperationId::Add},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Projection},
};
constexpr MenuEntry kAddSubMenuEntries[] = {
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Add},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Sub},
};
constexpr MenuEntry kRefRrefMenuEntries[] = {
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Ref},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::Rref},
};
constexpr MenuEntry kSpanMenuEntries[] = {
        {TextId::MenuEntryInSpanSolve, MenuEntryKind::Operation, MenuId::Main, OperationId::SolveRref},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::SpanTest},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::IndepTest},
        {TextId::MenuEntryBasisFromSet, MenuEntryKind::Operation, MenuId::Main, OperationId::ColSpaceBasis},
};
constexpr MenuEntry kSpacesMenuEntries[] = {
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::ColSpaceBasis},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::RowSpaceBasis},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::NullSpaceBasis},
        {TextId::None, MenuEntryKind::Operation, MenuId::Main, OperationId::LeftNullSpaceBasis},
};

constexpr std::uint8_t kMenuMaxEntries = [] {
		std::uint8_t count = 0;
		for (const auto n : {array_count(kMainMenuEntries), array_count(kMatricesMenuEntries), array_count(kOperationsMenuEntries),
		        array_count(kAddSubMenuEntries), array_count(kRefRrefMenuEntries), array_count(kSpanMenuEntries), array_count(kSpacesMenuEntries)})
				count = std::max(count, n);
		return count;
}();

std::uint8_t visible_menu(MenuId id, const MenuEntry** out, TextId& title) noexcept {
		const MenuEntry* entries = nullptr;
		std::uint8_t count = 0;
		switch (id) {
		case MenuId::Main:
				title = TextId::MenuMainTitle;
				entries = kMainMenuEntries;
				count = array_count(kMainMenuEntries);
				break;
		case MenuId::Matrices:
				title = TextId::MenuMatricesTitle;
				entries = kMatricesMenuEntries;
				count = array_count(kMatricesMenuEntries);
				break;
		case MenuId::Operations:
				title = TextId::MenuOperationsTitle;
				entries = kOperationsMenuEntries;
				count = array_count(kOperationsMenuEntries);
				break;
		case MenuId::AddSub:
				title = TextId::MenuAddSubTitle;
				entries = kAddSubMenuEntries;
				count = array_count(kAddSubMenuEntries);
				break;
		case MenuId::RefRref:
				title = TextId::MenuRefRrefTitle;
				entries = kRefRrefMenuEntries;
				count = array_count(kRefRrefMenuEntries);
				break;
		case MenuId::Span:
				title = TextId::MenuSpanTitle;
				entries = kSpanMenuEntries;
				count = array_count(kSpanMenuEntries);
				break;
		case MenuId::Spaces:
				title = TextId::MenuSpacesTitle;
				entries = kSpacesMenuEntries;
				count = array_count(kSpacesMenuEntries);
				break;
		case MenuId::SlotList:
				title = TextId::MenuEditMatrixTitle;
				return kSlotCount;
		default:
				detail::fail_fast("Unhandled MenuId");
		}
		std::uint8_t visible = 0;
		for (std::uint8_t i = 0; i < count; i++) {
				const MenuEntry& entry = entries[i];
				if (entry.kind == MenuEntryKind::Submenu || detail::op_enabled(entry.op))
						out[visible++] = &entry;
		}
		return visible;
}
}

void App::render_menu(const MenuState& s) noexcept {
		const MenuEntry* entries[kMenuMaxEntries];
		TextId title;
		const std::uint8_t count = visible_menu(s.id, entries, title);
		render_header(tr(title));
		render_footer_hint(tr(s.id == MenuId::Main ? TextId::FooterSelectExit :
		        s.id == MenuId::SlotList ? TextId::FooterEditSlotMenu : TextId::FooterSelectBack));
		for (std::uint8_t row = 0; row < ui::kListVisible && s.scroll + row < count; row++) {
				const std::uint8_t idx = static_cast<std::uint8_t>(s.scroll + row);
				ui::list_row(row, idx == s.cursor);
				if (s.id == MenuId::SlotList) {
						detail::render_slot(idx, slots_[idx]);
				} else {
						const MenuEntry& entry = *entries[idx];
						gfx_PrintString(entry.label == TextId::None ? detail::op_name(entry.op) : tr(entry.label));
				}
		}
}

void App::update_menu(MenuState& s) noexcept {
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				pop();
				return;
		}
		const MenuEntry* entries[kMenuMaxEntries];
		TextId title;
		const std::uint8_t count = visible_menu(s.id, entries, title);
		detail::update_list(input_, s.cursor, s.scroll, count);
		if (s.id == MenuId::SlotList) {
				const std::uint8_t slot = s.cursor;
				const Slot& sl = slots_[slot];
				if (input_.pressed(kKbGroup1, kb_2nd)) {
						push(sl.is_set() ? Page::make_confirm(slot, ConfirmAction::Resize) : Page::make_dim(slot, 2, 2));
						return;
				}
				if (input_.pressed(kKbGroup1, kb_Del)) {
						if (sl.is_set())
								push(Page::make_confirm(slot, ConfirmAction::Clear));
						else
								show_message(tr(TextId::CommonSlotUnset));
						return;
				}
				if (input_.pressed(kKbGroup6, kb_Enter))
						push(sl.is_set() ? Page::make_editor(slot) : Page::make_dim(slot, 2, 2));
				return;
		}
		if (!input_.pressed(kKbGroup6, kb_Enter) || count == 0)
				return;
		const MenuEntry& entry = *entries[s.cursor];
		push(entry.kind == MenuEntryKind::Submenu ? Page::make_menu(entry.submenu) : Page::make_slot_pick(entry.op));
}

}
