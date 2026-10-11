#include "matrix_shell/app.hpp"

namespace matrix_shell {

Page Page::make_menu(MenuId id) noexcept {
		Page p{};
		p.kind = PageKind::Menu;
		p.u.menu = MenuState{id, 0, 0};
		return p;
}

Page Page::make_dim(std::uint8_t slot, std::uint8_t rows, std::uint8_t cols) noexcept {
		Page p{};
		p.kind = PageKind::Dim;
		p.u.dim = DimState{slot, rows, cols};
		return p;
}

Page Page::make_editor(std::uint8_t slot) noexcept {
		Page p{};
		p.kind = PageKind::Editor;
		EditorState s;
		s.slot = slot;
		s.cur_r = 0;
		s.cur_c = 0;
		s.editing = false;
		s.edit_buf[0] = '\0';
		s.edit_len = 0;
		p.u.editor = s;
		return p;
}

Page Page::make_confirm(std::uint8_t slot, ConfirmAction action) noexcept {
		Page p{};
		p.kind = PageKind::Confirm;
		p.u.confirm = ConfirmState{slot, action};
		return p;
}

Page Page::make_slot_pick(OperationId op) noexcept {
		Page p{};
		p.kind = PageKind::SlotPick;
		p.u.pick = SlotPickState{op, 0, 0, 0, 0};
		return p;
}


Page Page::make_cramer_steps_menu(std::uint8_t slot_a, std::uint8_t slot_b, std::uint8_t n) noexcept {
		Page p{};
		p.kind = PageKind::CramerStepsMenu;
		p.u.cramer_steps = CramerStepsMenuState{slot_a, slot_b, n, 0, 0};
		return p;
}

Page Page::make_steps() noexcept {
		Page p{};
		p.kind = PageKind::Steps;
		p.u.steps = StepsState{0};
		return p;
}

Page Page::make_cofactor_element(std::uint8_t slot, std::uint8_t n) noexcept {
		Page p{};
		p.kind = PageKind::CofactorElement;
		p.u.cofactor = CofactorElementState{slot, n, 0, 0, 0};
		return p;
}


} // namespace matrix_shell
