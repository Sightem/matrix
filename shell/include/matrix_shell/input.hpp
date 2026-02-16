#pragma once

#include <cstddef>
#include <cstdint>

namespace matrix_shell {

struct RepeatConfig {
		std::uint16_t initial_delay_frames = 16;
		std::uint16_t repeat_period_frames = 3;
};

class Input {
	  public:
		void begin_frame() noexcept; // calls kb_Scan()

		bool down(std::uint8_t group, std::uint8_t mask) const noexcept;
		bool pressed(std::uint8_t group, std::uint8_t mask) const noexcept;
		bool released(std::uint8_t group, std::uint8_t mask) const noexcept;

		bool repeat(std::uint8_t group, std::uint8_t mask, RepeatConfig cfg = {}) noexcept;

	  private:
		std::uint8_t prev_[8]{};
		std::uint8_t cur_[8]{};

		struct RepeatState {
				bool held = false;
				std::uint16_t frames = 0;
		};

		RepeatState rep_up_{};
		RepeatState rep_down_{};
		RepeatState rep_left_{};
		RepeatState rep_right_{};

		RepeatState& rep_state_for(std::uint8_t group, std::uint8_t mask) noexcept;
};

} // namespace matrix_shell
