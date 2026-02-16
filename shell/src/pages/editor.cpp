#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include "matrix_shell/detail/app_internal.hpp"

#include "matrix_core/writer.hpp"

#include <graphx.h>
#include <keypadc.h>

#include <cassert>
#include <cstring>

namespace matrix_shell {
namespace {
using detail::fail_fast;
using detail::kKbGroup1;
using detail::kKbGroup3;
using detail::kKbGroup4;
using detail::kKbGroup5;
using detail::kKbGroup6;
using detail::kKbGroupArrows;
using detail::kScreenH;
using detail::kScreenW;
} // namespace

void App::render_editor(const EditorState& s) noexcept {
		if (s.slot >= kSlotCount)
				fail_fast("render_editor: invalid slot");
		const Slot& slot = slots_[s.slot];
		if (!slot.is_set())
				fail_fast("render_editor: slot is unset");
		render_header(nullptr);
		gfx_PrintChar(static_cast<char>('A' + s.slot));
		gfx_PrintChar(' ');
		gfx_PrintChar(static_cast<char>('0' + slot.rows));
		gfx_PrintChar('x');
		gfx_PrintChar(static_cast<char>('0' + slot.cols));

		if (s.editing)
				render_footer_hint("Digits  ENTER: OK  DEL: Back  CLEAR: Cancel");
		else
				render_footer_hint("ARROWS: Move  ENTER: Edit  DEL: 0  CLEAR: Back");

		const ui::Layout l = ui::Layout{};

		const int grid_top = l.header_h + 8;
		const int grid_left = l.margin_x;
		const int max_cols = 6;
		const int max_rows = 6;

		const int cell_w = (kScreenW - 2 * l.margin_x) / max_cols;
		const int cell_h = (kScreenH - l.header_h - l.footer_h - 2 * l.margin_y) / max_rows;

		const std::uint8_t rows = slot.rows;
		const std::uint8_t cols = slot.cols;

		for (std::uint8_t r = 0; r < rows; r++) {
				for (std::uint8_t c = 0; c < cols; c++) {
						const int x = grid_left + c * cell_w;
						const int y = grid_top + r * cell_h;
						const bool sel = (r == s.cur_r && c == s.cur_c) && !s.editing;

						if (sel) {
								gfx_SetColor(ui::color::kBlue);
								gfx_FillRectangle(x, y, cell_w - 1, cell_h - 1);
								gfx_SetTextFGColor(ui::color::kWhite);
						} else {
								gfx_SetColor(ui::color::kLightGray);
								gfx_FillRectangle(x, y, cell_w - 1, cell_h - 1);
								gfx_SetTextFGColor(ui::color::kBlack);
						}

						gfx_SetColor(ui::color::kDarkGray);
						gfx_Rectangle(x, y, cell_w - 1, cell_h - 1);

						// value
						const matrix_core::Rational& v = slot.view_active().at(r, c);
						fmt_buf_[0] = '\0';
						matrix_core::CheckedWriter w{fmt_buf_, sizeof(fmt_buf_)};
						w.append_i64(v.num()); // editor uses integer-only values

						gfx_SetTextXY(x + 3, y + 3);
						gfx_PrintString(fmt_buf_);
				}
		}

		// editing overlay
		if (s.editing) {
				const int y = kScreenH - l.footer_h - 18;
				gfx_SetColor(ui::color::kBlue);
				gfx_FillRectangle(0, y, kScreenW, 18);
				gfx_SetTextFGColor(ui::color::kWhite);
				gfx_SetTextXY(l.margin_x, y + 4);
				gfx_PrintString("Value: ");
				gfx_PrintString(s.edit_buf);
		}
}

void App::update_editor(EditorState& s) noexcept {
		if (s.slot >= kSlotCount) {
				REQUIRE(pop(), "pop failed (invalid slot)");
				return;
		}

		Slot& slot = slots_[s.slot];
		if (!slot.is_set()) {
				REQUIRE(pop(), "pop failed (unset slot)");
				return;
		}

		if (s.editing) {
				assert(s.edit_len < sizeof(s.edit_buf));
				assert(s.edit_buf[s.edit_len] == '\0');

				// cancel edit
				if (input_.pressed(kKbGroup6, kb_Clear)) {
						SHELL_DBG("[edit] cancel slot=%c cell=(%u,%u) buf=\"%s\"\n",
						        (char)('A' + s.slot),
						        (unsigned)s.cur_r,
						        (unsigned)s.cur_c,
						        s.edit_buf);
						s.editing = false;
						return;
				}

				// backspace
				if (input_.pressed(kKbGroup1, kb_Del)) {
						if (s.edit_len > 0) {
								s.edit_len--;
								s.edit_buf[s.edit_len] = '\0';
						}
						return;
				}

				// toggle sign
				if (input_.pressed(kKbGroup5, kb_Chs)) {
						if (s.edit_len == 0) {
								s.edit_buf[0] = '-';
								s.edit_buf[1] = '\0';
								s.edit_len = 1;
						} else if (s.edit_buf[0] == '-') {
								assert(s.edit_len > 0);
								std::memmove(s.edit_buf, s.edit_buf + 1, static_cast<std::size_t>(s.edit_len));
								s.edit_len--;
								s.edit_buf[s.edit_len] = '\0';
						} else {
								// insert leading '-'
								if (s.edit_len + 1 < sizeof(s.edit_buf)) {
										std::memmove(s.edit_buf + 1, s.edit_buf, static_cast<std::size_t>(s.edit_len + 1));
										s.edit_buf[0] = '-';
										s.edit_len++;
										s.edit_buf[s.edit_len] = '\0';
								}
						}
						return;
				}

				auto push_digit = [&](char d) noexcept {
						assert(s.edit_len < sizeof(s.edit_buf));
						assert(s.edit_buf[s.edit_len] == '\0');
						if (s.edit_len + 1 >= sizeof(s.edit_buf))
								return;
						s.edit_buf[s.edit_len++] = d;
						s.edit_buf[s.edit_len] = '\0';
				};

				if (input_.pressed(kKbGroup3, kb_0))
						push_digit('0');
				if (input_.pressed(kKbGroup3, kb_1))
						push_digit('1');
				if (input_.pressed(kKbGroup4, kb_2))
						push_digit('2');
				if (input_.pressed(kKbGroup5, kb_3))
						push_digit('3');
				if (input_.pressed(kKbGroup3, kb_4))
						push_digit('4');
				if (input_.pressed(kKbGroup4, kb_5))
						push_digit('5');
				if (input_.pressed(kKbGroup5, kb_6))
						push_digit('6');
				if (input_.pressed(kKbGroup3, kb_7))
						push_digit('7');
				if (input_.pressed(kKbGroup4, kb_8))
						push_digit('8');
				if (input_.pressed(kKbGroup5, kb_9))
						push_digit('9');

				if (!input_.pressed(kKbGroup6, kb_Enter))
						return;

				std::int64_t v = 0;
				if (s.edit_len == 0) {
						s.editing = false;
						return;
				}
				if (!parse_i64(s.edit_buf, &v)) {
						show_message("Invalid integer");
						s.editing = false;
						return;
				}

				slot.view_active_mut().at_mut(s.cur_r, s.cur_c) = matrix_core::Rational::from_int(v);
					SHELL_DBG("[edit] commit slot=%c cell=(%u,%u) v=", (char)('A' + s.slot), (unsigned)s.cur_r, (unsigned)s.cur_c);
					detail::dbg_print_i64(v);
					SHELL_DBG("\n");
					s.editing = false;
					return;
			}

		// non-editing navigation/back
		if (input_.pressed(kKbGroup6, kb_Clear)) {
				SHELL_DBG("[edit] CLEAR back slot=%c\n", (char)('A' + s.slot));
				REQUIRE(pop(), "pop failed");
				return;
		}

		const std::uint8_t max_r = static_cast<std::uint8_t>(slot.rows - 1);
		const std::uint8_t max_c = static_cast<std::uint8_t>(slot.cols - 1);

		if (input_.repeat(kKbGroupArrows, kb_Left)) {
				if (s.cur_c > 0)
						s.cur_c--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Right)) {
				if (s.cur_c < max_c)
						s.cur_c++;
		}
		if (input_.repeat(kKbGroupArrows, kb_Up)) {
				if (s.cur_r > 0)
						s.cur_r--;
		}
		if (input_.repeat(kKbGroupArrows, kb_Down)) {
				if (s.cur_r < max_r)
						s.cur_r++;
		}

		// quick clear cell to 0
		if (input_.pressed(kKbGroup1, kb_Del)) {
				slot.view_active_mut().at_mut(s.cur_r, s.cur_c) = matrix_core::Rational::from_int(0);
				SHELL_DBG("[edit] DEL zero slot=%c cell=(%u,%u)\n", (char)('A' + s.slot), (unsigned)s.cur_r, (unsigned)s.cur_c);
				return;
		}

		// enter edit
		if (input_.pressed(kKbGroup6, kb_Enter)) {
				s.editing = true;
				s.edit_len = 0;
				s.edit_buf[0] = '\0';
				SHELL_DBG("[edit] begin slot=%c cell=(%u,%u)\n", (char)('A' + s.slot), (unsigned)s.cur_r, (unsigned)s.cur_c);
				return;
		}
}

} // namespace matrix_shell
