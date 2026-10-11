#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/error.hpp"
#include "matrix_core/matrix.hpp"

namespace matrix_core {
struct StepRenderBuffers {
		char* caption = nullptr;
		std::size_t caption_cap = 0;
		char* latex = nullptr;
		std::size_t latex_cap = 0;
		MatrixMutView work{};
};

struct Explanation {
		enum class Kind : std::uint8_t {
				None, Add, Sub, Mul, Transpose, Ref, Rref, Det, Inverse,
				Cofactor, DetReplace, Dot, Cross, Projection,
		};

		Kind kind = Kind::None;
		MatrixView input{};
		MatrixView other{};
		MatrixView result{};
		Rational value = Rational::from_int(0);
		Rational dot_vv = Rational::from_int(0);
		Rational k = Rational::from_int(0);
		Rational proj_norm2 = Rational::from_int(0);
		Rational orth_norm2 = Rational::from_int(0);
		std::size_t op_count = 0;
		std::uint8_t row = 0;
		std::uint8_t col = 0;

		bool available() const noexcept { return kind != Kind::None; }
		std::size_t step_count() const noexcept;
		ErrorCode render_step(std::size_t index, const StepRenderBuffers& out) const noexcept;

	private:
		ErrorCode render_binary(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_transpose(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_echelon(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_det(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_inverse(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_cofactor(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_det_replace(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_dot(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_cross(std::size_t index, const StepRenderBuffers& out) const noexcept;
		ErrorCode render_proj(std::size_t index, const StepRenderBuffers& out) const noexcept;
};
} // namespace matrix_core
