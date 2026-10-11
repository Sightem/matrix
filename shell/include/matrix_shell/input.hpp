#pragma once

#include <cstdint>

namespace matrix_shell {

class Input {
	  public:
		void begin_frame() noexcept; // calls kb_Scan()

		bool down(std::uint8_t group, std::uint8_t mask) const noexcept;
		bool pressed(std::uint8_t group, std::uint8_t mask) const noexcept;

		bool repeat(std::uint8_t group, std::uint8_t mask) noexcept;

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

		RepeatState& rep_state_for(std::uint8_t mask) noexcept;
};

} // namespace matrix_shell
