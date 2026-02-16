#include "matrix_shell/input.hpp"

#include <keypadc.h>

namespace matrix_shell {

void Input::begin_frame() noexcept {
		for (std::uint8_t i = 0; i < 8; i++)
				prev_[i] = cur_[i];

		kb_Scan();
		for (std::uint8_t i = 0; i < 8; i++)
				cur_[i] = kb_Data[i];
}

bool Input::down(std::uint8_t group, std::uint8_t mask) const noexcept {
		if (group >= 8)
				return false;
		return (cur_[group] & mask) != 0;
}

bool Input::pressed(std::uint8_t group, std::uint8_t mask) const noexcept {
		if (group >= 8)
				return false;
		return (cur_[group] & mask) != 0 && (prev_[group] & mask) == 0;
}

bool Input::released(std::uint8_t group, std::uint8_t mask) const noexcept {
		if (group >= 8)
				return false;
		return (cur_[group] & mask) == 0 && (prev_[group] & mask) != 0;
}

Input::RepeatState& Input::rep_state_for(std::uint8_t group, std::uint8_t mask) noexcept {
		// Only arrows repeat.
		if (group == 7 && mask == kb_Up)
				return rep_up_;
		if (group == 7 && mask == kb_Down)
				return rep_down_;
		if (group == 7 && mask == kb_Left)
				return rep_left_;
		return rep_right_;
}

bool Input::repeat(std::uint8_t group, std::uint8_t mask, RepeatConfig cfg) noexcept {
		RepeatState& st = rep_state_for(group, mask);
		const bool is_down = down(group, mask);
		const bool was_down = (group < 8) ? ((prev_[group] & mask) != 0) : false;

		if (!is_down) {
				st.held = false;
				st.frames = 0;
				return false;
		}

		// initial press always counts
		if (is_down && !was_down) {
				st.held = true;
				st.frames = 0;
				return true;
		}

		// count frames and fire repeats
		if (!st.held) {
				st.held = true;
				st.frames = 0;
				return false;
		}

		st.frames++;
		if (st.frames < cfg.initial_delay_frames)
				return false;

		const std::uint16_t after = static_cast<std::uint16_t>(st.frames - cfg.initial_delay_frames);
		return (after % cfg.repeat_period_frames) == 0;
}

} // namespace matrix_shell
