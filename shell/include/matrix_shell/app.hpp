#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/arena.hpp"
#include "matrix_core/explanation.hpp"
#include "matrix_core/matrix.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/slab.hpp"

#include "matrix_shell/input.hpp"
#include "matrix_shell/config.hpp"

struct TeX_Layout;
struct TeX_Renderer;

namespace matrix_shell {

constexpr std::size_t kMaxPageDepth = 8;
constexpr std::uint8_t kSlotCount = 8;

enum class MenuId : std::uint8_t {
		Main,
		Matrices,
		Operations,
		Span,
		Spaces,
		AddSub,
		RefRref,
		SlotList, // A..H (edit)
};

enum class OperationId : std::uint8_t {
		Add,
		Sub,
		Mul,
		Dot,
		Cross,
		Projection,
		Cramer,
		Det,
		SolveRref,
		SpanTest,
		IndepTest,
		Transpose,
		Inverse,
		ColSpaceBasis,
		RowSpaceBasis,
		NullSpaceBasis,
		LeftNullSpaceBasis,
		MinorMatrix,
		CofactorElement,
		Ref,
		Rref,
};

struct MenuState {
		MenuId id;
		std::uint8_t cursor;
		std::uint8_t scroll;
};

struct DimState {
		std::uint8_t slot;
		std::uint8_t rows;
		std::uint8_t cols;
};

struct EditorState {
		std::uint8_t slot;
		std::uint8_t cur_r;
		std::uint8_t cur_c;

		bool editing;
		char edit_buf[20];
		std::uint8_t edit_len;
};

struct SlotPickState {
		OperationId op;
		std::uint8_t stage;  // 0=pick first, 1=pick second (binary ops)
		std::uint8_t slot_a; // valid when stage==1 or after selection
		std::uint8_t cursor;
		std::uint8_t scroll;
};

struct ResultState {
		OperationId op;
		std::uint8_t slot_a;
		std::uint8_t slot_b;
		bool is_scalar;
		bool has_steps;

		std::uint8_t rows;
		std::uint8_t cols;
		std::uint8_t stride;
		const matrix_core::Rational* data;

		// for scalar results that depend on element selection (cofactor element)
		std::uint8_t i;
		std::uint8_t j;

		std::int64_t num;
		std::int64_t den;
};

enum class ConfirmAction : std::uint8_t {
		Resize,
		Clear,
};

struct ConfirmState {
		std::uint8_t slot;
		ConfirmAction action;
};

struct MessageState {
		bool active;
		std::uint16_t frames_left;
		char text[64];
};

enum class PageKind : std::uint8_t {
		Menu,
		Dim,
		Editor,
		SlotPick,
		CofactorElement,
		Result,
		ProjectionResult,
		CramerStepsMenu,
		Steps,
		Confirm,
};

struct CofactorElementState {
		std::uint8_t slot;
		std::uint8_t n;     // square size
		std::uint8_t i;     // 0-based
		std::uint8_t j;     // 0-based
		std::uint8_t focus; // 0=i, 1=j
};

struct ProjectionResultState {
		std::uint8_t slot_u;
		std::uint8_t slot_v;
		std::uint8_t mode; // 0=proj, 1=orth
		bool has_steps;

		std::uint8_t rows;
		std::uint8_t cols;
		std::uint8_t stride_proj;
		std::uint8_t stride_orth;
		const matrix_core::Rational* proj_data;
		const matrix_core::Rational* orth_data;

		std::int64_t k_num;
		std::int64_t k_den;
};

struct CramerStepsMenuState {
		std::uint8_t slot_a;
		std::uint8_t slot_b;
		std::uint8_t n;
		std::uint8_t cursor;
		std::uint8_t scroll;
};

struct StepsState {
		std::uint16_t index;
};

struct Page {
		PageKind kind;

		union {
				MenuState menu;
				DimState dim;
				EditorState editor;
				SlotPickState pick;
				CofactorElementState cofactor;
				ResultState result;
				ProjectionResultState proj;
				CramerStepsMenuState cramer_steps;
				StepsState steps;
				ConfirmState confirm;
		} u;

		static Page make_menu(MenuId id) noexcept;
		static Page make_dim(std::uint8_t slot, std::uint8_t rows, std::uint8_t cols) noexcept;
		static Page make_editor(std::uint8_t slot) noexcept;
		static Page make_slot_pick(OperationId op) noexcept;
		static Page make_cofactor_element(std::uint8_t slot, std::uint8_t n) noexcept;
		static Page make_result_matrix(OperationId op,
		        std::uint8_t slot_a,
		        std::uint8_t slot_b,
		        bool has_steps,
		        std::uint8_t rows,
		        std::uint8_t cols,
		        std::uint8_t stride,
		        const matrix_core::Rational* data) noexcept;
		static Page make_result_scalar(
		        OperationId op, std::uint8_t slot_a, std::uint8_t slot_b, bool has_steps, std::int64_t num, std::int64_t den) noexcept;
		static Page make_projection_result(std::uint8_t slot_u,
		        std::uint8_t slot_v,
		        bool has_steps,
		        std::uint8_t rows,
		        std::uint8_t cols,
		        std::uint8_t stride_proj,
		        const matrix_core::Rational* proj_data,
		        std::uint8_t stride_orth,
		        const matrix_core::Rational* orth_data,
		        std::int64_t k_num,
		        std::int64_t k_den) noexcept;
		static Page make_cramer_steps_menu(std::uint8_t slot_a, std::uint8_t slot_b, std::uint8_t n) noexcept;
		static Page make_result_cofactor_element(
		        std::uint8_t slot_a, std::uint8_t i, std::uint8_t j, bool has_steps, std::int64_t num, std::int64_t den) noexcept;
		static Page make_steps() noexcept;
		static Page make_confirm(std::uint8_t slot, ConfirmAction action) noexcept;
};

struct Slot {
		matrix_core::MatrixMutView backing{}; // always 6x6 when allocated
		std::uint8_t rows = 0;                // active
		std::uint8_t cols = 0;                // active

		bool allocated() const noexcept { return backing.data != nullptr; }
		bool is_set() const noexcept { return allocated() && rows >= 1 && cols >= 1; }

		matrix_core::MatrixView view_active() const noexcept { return {rows, cols, backing.stride, backing.data}; }
		matrix_core::MatrixMutView view_active_mut() noexcept { return {rows, cols, backing.stride, backing.data}; }
};

class App {
	  public:
		bool init() noexcept;
		bool step() noexcept;

	  private:
		matrix_core::Slab slab_{};
		matrix_core::Arena persist_{};
		matrix_core::Arena scratch_{};
		std::size_t persist_base_mark_ = 0;
		std::size_t persist_tail_mark_ = 0;

		Slot slots_[kSlotCount]{};
		Page stack_[kMaxPageDepth]{};
		std::uint8_t depth_ = 0;

		Input input_{};
		MessageState msg_{};
		char fmt_buf_[64]{};
		char step_caption_[128]{};
		char step_latex_[1024]{};
		matrix_core::Explanation expl_{};

		TeX_Renderer* tex_renderer_ = nullptr;
		TeX_Layout* tex_layout_ = nullptr;
		std::uint16_t tex_cached_step_index_ = 0xFFFF;
		matrix_core::ErrorCode tex_cached_step_ec_ = matrix_core::ErrorCode::Internal;
		int tex_scroll_y_ = 0;
		int tex_total_height_ = 0;
		char tex_doc_[1400]{};

		Page& top() noexcept;
		const Page& top() const noexcept;
		[[nodiscard]] bool push(const Page& p) noexcept;
		[[nodiscard]] bool pop() noexcept;

		void push_root() noexcept;

		void render() noexcept;
		void render_header(const char* title) noexcept;
		void render_footer_hint(const char* hint) noexcept;
		void render_message() noexcept;

		void render_menu(const MenuState& s) noexcept;
		void render_dim(const DimState& s) noexcept;
		void render_editor(const EditorState& s) noexcept;
		void render_slot_pick(const SlotPickState& s) noexcept;
		void render_result(const ResultState& s) noexcept;
		void render_projection_result(const ProjectionResultState& s) noexcept;
		void render_cramer_steps_menu(const CramerStepsMenuState& s) noexcept;
		void render_steps(const StepsState& s) noexcept;
		void render_cofactor_element(const CofactorElementState& s) noexcept;
		void render_confirm(const ConfirmState& s) noexcept;

		void update_message() noexcept;
		void show_message(const char* msg) noexcept;

		void update_menu(MenuState& s) noexcept;
		void update_dim(DimState& s) noexcept;
		void update_editor(EditorState& s) noexcept;
		void update_slot_pick(SlotPickState& s) noexcept;
		void update_cofactor_element(CofactorElementState& s) noexcept;
		void update_result(ResultState& s) noexcept;
		void update_projection_result(ProjectionResultState& s) noexcept;
		void update_cramer_steps_menu(CramerStepsMenuState& s) noexcept;
		void update_steps(StepsState& s) noexcept;
		void update_confirm(ConfirmState& s) noexcept;

		matrix_core::ErrorCode ensure_slot_allocated(std::uint8_t slot) noexcept;
		void clear_slot(std::uint8_t slot) noexcept;

		static bool parse_i64(const char* s, std::int64_t* out) noexcept;

		void steps_tex_reset() noexcept;
		void steps_tex_release() noexcept;
};

} // namespace matrix_shell
