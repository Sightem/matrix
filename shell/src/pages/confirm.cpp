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
} // namespace

void App::render_confirm(const ConfirmState& s) noexcept {
		render_header(tr(TextId::CommonConfirmTitle));
		render_footer_hint(tr(TextId::FooterYesNo));


		gfx_SetTextFGColor(ui::color::kBlack);
		gfx_SetTextXY(12, 60);

		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintString(": ");

		switch (s.action) {
		case ConfirmAction::Resize:
				gfx_PrintString(tr(TextId::ConfirmResizePrompt));
				break;
		case ConfirmAction::Clear:
				gfx_PrintString(tr(TextId::ConfirmClearPrompt));
				break;
		default:
				fail_fast("render_confirm: unhandled ConfirmAction");
		}
}

void App::update_confirm(ConfirmState& s) noexcept {
		// CLEAR: no
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[confirm] CLEAR cancel action=%u slot=%c\n", (unsigned)s.action, (char)('A' + s.slot));
				pop();
				return;
		}

		// ENTER: yes
		if (!input_.pressed(kKbGroup6, kb_Enter))
				return;


		switch (s.action) {
		case ConfirmAction::Clear:
				SHELL_DBG("[confirm] ENTER clear slot=%c\n", (char)('A' + s.slot));
				clear_slot(s.slot);
				show_message(tr(TextId::CommonCleared));
				pop();
				return;
		case ConfirmAction::Resize:
				// resize: go to dim page
				SHELL_DBG("[confirm] ENTER resize slot=%c -> dim 2x2\n", (char)('A' + s.slot));
				pop();
				push(Page::make_dim(s.slot, 2, 2));
				return;
		}

		fail_fast("Unhandled ConfirmAction in update_confirm");
}

} // namespace matrix_shell
