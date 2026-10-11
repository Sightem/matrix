#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/explanation.hpp"
#include "matrix_core/matrix.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/spaces.hpp"

#include "matrix_shell/input.hpp"
#include "matrix_shell/config.hpp"

#include "tex/tex.h"

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

struct SpaceSummary {
		std::uint8_t rank;
		std::uint8_t nullity;
		std::uint32_t pivot_mask;
};

struct PredicateResult {
		bool value;
		matrix_core::Dim input;
		SpaceSummary space;
};

struct BasisResult {
		matrix_core::MatrixView vectors;
		SpaceSummary space;
};

struct SolutionResult {
		matrix_core::MatrixView particular;
		matrix_core::MatrixView null_basis;
		SpaceSummary space;
		bool show_null_basis;
};

struct CofactorResult {
		matrix_core::Rational value;
		std::uint8_t row;
		std::uint8_t col;
};

struct ProjectionResult {
		matrix_core::MatrixView projected;
		matrix_core::MatrixView orthogonal;
		matrix_core::Rational factor;
		bool show_orthogonal;
};

struct ResultState {
		OperationId op;
		std::uint8_t slot_a;
		std::uint8_t slot_b;
		bool has_steps;
		union {
				matrix_core::Rational scalar;
				matrix_core::MatrixView matrix;
				PredicateResult predicate;
				BasisResult basis;
				SolutionResult solution;
				CofactorResult cofactor;
				ProjectionResult projection;
		} value;
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
				CramerStepsMenuState cramer_steps;
				StepsState steps;
				ConfirmState confirm;
		} u;

		static Page make_menu(MenuId id) noexcept;
		static Page make_dim(std::uint8_t slot, std::uint8_t rows, std::uint8_t cols) noexcept;
		static Page make_editor(std::uint8_t slot) noexcept;
		static Page make_slot_pick(OperationId op) noexcept;
		static Page make_cofactor_element(std::uint8_t slot, std::uint8_t n) noexcept;
		static Page make_cramer_steps_menu(std::uint8_t slot_a, std::uint8_t slot_b, std::uint8_t n) noexcept;
		static Page make_steps() noexcept;
		static Page make_confirm(std::uint8_t slot, ConfirmAction action) noexcept;
};

struct Slot {
		matrix_core::Rational data[36]{};
		std::uint8_t rows = 0;
		std::uint8_t cols = 0;

		bool is_set() const noexcept { return rows != 0 && cols != 0; }
		matrix_core::MatrixView view_active() const noexcept { return {rows, cols, 6, data}; }
		matrix_core::MatrixMutView view_active_mut() noexcept { return {rows, cols, 6, data}; }
};

struct Calculation {
		matrix_core::Rational result[6 + 36]{};
		matrix_core::Rational input[6 * 7]{};
		matrix_core::Explanation explanation{};
};

class App {
	  public:
		bool init() noexcept;
		bool step() noexcept;

	  private:
		Calculation calculation_{};
		matrix_core::Rational work_[6 * 12]{};
		matrix_core::MatrixMutView work() noexcept { return {6, 12, 12, work_}; }

		Slot slots_[kSlotCount]{};
		Page stack_[kMaxPageDepth]{};
		std::uint8_t depth_ = 0;

		Input input_{};
		MessageState msg_{};
		char fmt_buf_[64]{};
		char step_caption_[128]{};
		char step_latex_[1024]{};

		TeX_Config tex_config_{};
		TeX_Renderer* tex_renderer_ = nullptr;
		TeX_Layout* tex_layout_ = nullptr;
		std::uint16_t tex_cached_step_index_ = 0xFFFF;
		matrix_core::ErrorCode tex_cached_step_ec_ = matrix_core::ErrorCode::Internal;
		int tex_scroll_y_ = 0;
		int tex_total_height_ = 0;
		char tex_doc_[1400]{};

		Page& top() noexcept;
		void push(const Page& p) noexcept;
		void pop() noexcept;

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
		void render_result_grid(matrix_core::MatrixView matrix, int top) noexcept;
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
		void execute_operation(OperationId op, std::uint8_t slot_a, std::uint8_t slot_b = 0,
		        std::uint8_t row = 0, std::uint8_t col = 0) noexcept;
		bool operation_ok(OperationId op, matrix_core::ErrorCode error,
		        std::uint8_t slot_a, std::uint8_t slot_b = 0) noexcept;
		void update_result(ResultState& s) noexcept;
		void update_cramer_steps_menu(CramerStepsMenuState& s) noexcept;
		void update_steps(StepsState& s) noexcept;
		void update_confirm(ConfirmState& s) noexcept;

		void clear_slot(std::uint8_t slot) noexcept;

		static bool parse_i64(const char* s, std::int64_t* out) noexcept;

		void steps_tex_reset() noexcept;
		void steps_tex_release() noexcept;
};

} // namespace matrix_shell
