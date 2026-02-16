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

Page Page::make_result_matrix(OperationId op,
        std::uint8_t slot_a,
        std::uint8_t slot_b,
        bool has_steps,
        std::uint8_t rows,
        std::uint8_t cols,
        std::uint8_t stride,
        const matrix_core::Rational* data) noexcept {
		Page p{};
		p.kind = PageKind::Result;
		ResultState r;
		r.op = op;
		r.slot_a = slot_a;
		r.slot_b = slot_b;
		r.is_scalar = false;
		r.has_steps = has_steps;
		r.rows = rows;
		r.cols = cols;
		r.stride = stride;
		r.data = data;
		r.i = 0;
		r.j = 0;
		r.num = 0;
		r.den = 1;
		p.u.result = r;
		return p;
}

Page Page::make_result_scalar(
        OperationId op, std::uint8_t slot_a, std::uint8_t slot_b, bool has_steps, std::int64_t num, std::int64_t den) noexcept {
		Page p{};
		p.kind = PageKind::Result;
		ResultState r;
		r.op = op;
		r.slot_a = slot_a;
		r.slot_b = slot_b;
		r.is_scalar = true;
		r.has_steps = has_steps;
		r.rows = 0;
		r.cols = 0;
		r.stride = 0;
		r.data = nullptr;
		r.i = 0;
		r.j = 0;
		r.num = num;
		r.den = den;
		p.u.result = r;
		return p;
}

Page Page::make_projection_result(std::uint8_t slot_u,
        std::uint8_t slot_v,
        bool has_steps,
        std::uint8_t rows,
        std::uint8_t cols,
        std::uint8_t stride_proj,
        const matrix_core::Rational* proj_data,
        std::uint8_t stride_orth,
        const matrix_core::Rational* orth_data,
        std::int64_t k_num,
        std::int64_t k_den) noexcept {
		Page p{};
		p.kind = PageKind::ProjectionResult;
		p.u.proj = ProjectionResultState{
		        .slot_u = slot_u,
		        .slot_v = slot_v,
		        .mode = 0,
		        .has_steps = has_steps,
		        .rows = rows,
		        .cols = cols,
		        .stride_proj = stride_proj,
		        .stride_orth = stride_orth,
		        .proj_data = proj_data,
		        .orth_data = orth_data,
		        .k_num = k_num,
		        .k_den = k_den,
		};
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

Page Page::make_result_cofactor_element(
        std::uint8_t slot_a, std::uint8_t i, std::uint8_t j, bool has_steps, std::int64_t num, std::int64_t den) noexcept {
		Page p{};
		p.kind = PageKind::Result;
		ResultState r;
		r.op = OperationId::CofactorElement;
		r.slot_a = slot_a;
		r.slot_b = 0;
		r.is_scalar = true;
		r.has_steps = has_steps;
		r.rows = 0;
		r.cols = 0;
		r.stride = 0;
		r.data = nullptr;
		r.i = i;
		r.j = j;
		r.num = num;
		r.den = den;
		p.u.result = r;
		return p;
}

} // namespace matrix_shell
