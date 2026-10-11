#include "matrix_shell/input.hpp"

#include <keypadc.h>

namespace matrix_shell {
namespace {
constexpr std::uint16_t kInitialDelayFrames = 16;
constexpr std::uint16_t kRepeatPeriodFrames = 3;
}

void Input::begin_frame() noexcept {
		for (std::uint8_t i = 0; i < 8; i++)
				prev_[i] = cur_[i];

		kb_Scan();
		for (std::uint8_t i = 0; i < 8; i++)
				cur_[i] = kb_Data[i];
}

bool Input::down(std::uint8_t group, std::uint8_t mask) const noexcept {
		return (cur_[group] & mask) != 0;
}

bool Input::pressed(std::uint8_t group, std::uint8_t mask) const noexcept {
		return (cur_[group] & mask) != 0 && (prev_[group] & mask) == 0;
}

Input::RepeatState& Input::rep_state_for(std::uint8_t mask) noexcept {
		// Only arrows repeat.
		if (mask == kb_Up)
				return rep_up_;
		if (mask == kb_Down)
				return rep_down_;
		if (mask == kb_Left)
				return rep_left_;
		return rep_right_;
}

bool Input::repeat(std::uint8_t group, std::uint8_t mask) noexcept {
		RepeatState& st = rep_state_for(mask);
		const bool is_down = down(group, mask);
		const bool was_down = (prev_[group] & mask) != 0;

		if (!is_down) {
				st.held = false;
				st.frames = 0;
				return false;
		}

		// initial press always counts
		if (!was_down) {
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
		if (st.frames < kInitialDelayFrames)
				return false;

		const std::uint16_t after = static_cast<std::uint16_t>(st.frames - kInitialDelayFrames);
		return (after % kRepeatPeriodFrames) == 0;
}

} // namespace matrix_shell
