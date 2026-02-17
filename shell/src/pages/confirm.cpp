#include "matrix_shell/app.hpp"

#include "matrix_shell/text.hpp"
#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include <graphx.h>
#include <keypadc.h>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::kKbGroup6;
using namespace matrix_shell::text_literals;
} // namespace

void App::render_confirm(const ConfirmState& s) noexcept {
		render_header("common.confirm_title"_tx);
		render_footer_hint("footer.yes_no"_tx);

		if (s.slot >= kSlotCount)
				fail_fast("render_confirm: slot out of range");

		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(12, 60);

		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintString(": ");

		switch (s.action) {
		case ConfirmAction::Resize:
				gfx_PrintString("confirm.resize_prompt"_tx);
				break;
		case ConfirmAction::Clear:
				gfx_PrintString("confirm.clear_prompt"_tx);
				break;
		default:
				fail_fast("render_confirm: unhandled ConfirmAction");
		}
}

void App::update_confirm(ConfirmState& s) noexcept {
		// CLEAR: no
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[confirm] CLEAR cancel action=%u slot=%c\n", (unsigned)s.action, (char)('A' + s.slot));
				REQUIRE(pop(), "pop failed (cancel)");
				return;
		}

		// ENTER: yes
		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;

		if (s.slot >= kSlotCount)
				fail_fast("update_confirm: slot out of range");

		switch (s.action) {
		case ConfirmAction::Clear:
				SHELL_DBG("[confirm] ENTER clear slot=%c\n", (char)('A' + s.slot));
				clear_slot(s.slot);
				show_message("common.cleared"_tx);
				REQUIRE(pop(), "pop failed (clear)");
				return;
		case ConfirmAction::Resize:
				// resize: go to dim page
				SHELL_DBG("[confirm] ENTER resize slot=%c -> dim 2x2\n", (char)('A' + s.slot));
				REQUIRE(pop(), "pop failed (resize)");
				REQUIRE(push(Page::make_dim(s.slot, 2, 2)), "push dim failed");
				return;
		}

		fail_fast("Unhandled ConfirmAction in update_confirm");
}

} // namespace matrix_shell
