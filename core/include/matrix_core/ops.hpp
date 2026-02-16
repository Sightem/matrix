#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/arena.hpp"
#include "matrix_core/config.hpp"
#include "matrix_core/error.hpp"
#include "matrix_core/explanation.hpp"
#include "matrix_core/matrix.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/row_ops.hpp"

namespace matrix_core {
enum class ExplainDetail : std::uint8_t {
		Full = 0,
		Compact, // fewer, higher level steps
};

struct ExplainOptions {
		bool enable = false;
		Arena* persist = nullptr; // required when enable==true
		ExplainDetail detail = ExplainDetail::Full;
};

// indices in these APIs are 0 based (consistent with m.at(r, c))
// the shell/ui can present 1 based indices and convert as needed
//
// memory model:
// - matrix data must outlive any Explanation created from it
// - when opts.enable==true, opts.persist must be a valid long lived arena for
// the
//   explanation context
// - step rendering requires StepRenderBuffers::scratch to be a valid arena, it
// is
//   cleared by the renderer on each call
//
Error op_add(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_sub(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_mul(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_transpose(In MatrixView a, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

enum class EchelonKind : std::uint8_t {
		Ref,
		Rref,
};

Error op_echelon(
        In MatrixView a, In EchelonKind kind, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

Error op_det(In MatrixView a, InOut Arena& scratch, Out Rational* out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

// determinant of A with column "col" replaced by vector "b" (n x 1), useful for
// cramer's rule step breakdown (Δ_i)
Error op_det_replace_column(In MatrixView a,
        In MatrixView b,
        In std::uint8_t col,
        InOut Arena& scratch,
        Out Rational* out,
        Out Explanation* expl,
        In const ExplainOptions& opts) noexcept;

// element cofactor for square A (0 based indices)
//
// For A of size 1x1, C_{1,1} is defined as 1.
//
// when opts.enable==true, explanation provides:
//   A -> submatrix -> row ops -> final value
Error op_cofactor_element(In MatrixView a,
        In std::uint8_t i,
        In std::uint8_t j,
        InOut Arena& scratch,
        Out Rational* out,
        Out Explanation* expl,
        In const ExplainOptions& opts) noexcept;

// matrix of minors for square A (n>=2). output is n×n:
//   M[i,j] = det(A with row i and column j removed)
Error op_minor_matrix(In MatrixView a, InOut Arena& scratch, Out MatrixMutView out) noexcept;

// cramers rule solve (Ax=b), returning x as an n x 1 matrix. no step breakdown
// is produced here, shell can request Δ and Δ_i explanations via op_det()
// and op_det_replace_column()
Error op_cramer_solve(In MatrixView a, In MatrixView b, InOut Arena& scratch, Out MatrixMutView x_out) noexcept;

// inverse via gauss jordan elimination on the augmented matrix [A | I]
//
// on success, writes A^{-1} into out
// when opts.enable==true, explanation steps render the augmented matrix after
// each row operation
Error op_inverse(
        In MatrixView a, InOut Arena& scratch, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

/// Vector operations

// dot product of two vectors. vectors may be represented as n×1 or 1×n
Error op_dot(In MatrixView u, In MatrixView v, Out Rational* out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

// cross product of two 3D vectors. inputs may be 3×1 or 1×3
// output must have the same dimensions as u (same orientation)
Error op_cross(In MatrixView u, In MatrixView v, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

struct ProjDecomposeResult {
		Rational dot_uv = Rational::from_int(0);
		Rational dot_vv = Rational::from_int(0);
		Rational k = Rational::from_int(0);          // k = (u·v)/(v·v)
		Rational proj_norm2 = Rational::from_int(0); // ||proj||^2
		Rational orth_norm2 = Rational::from_int(0); // ||orth||^2
};

// decompose u into a component along v and an orthogonal remainder
//   proj = proj_v(u) = (u·v)/(v·v) * v
//   orth = u - proj
//
// u and v may be n×1 or 1×n (same length n in [1 to 6])
// out_proj and out_orth must have the same dimensions as u
// returns DivisionByZero if v is the zero vector (v·v == 0)
Error op_proj_decompose_u_onto_v(In MatrixView u,
        In MatrixView v,
        Out MatrixMutView out_proj,
        Out MatrixMutView out_orth,
        Out ProjDecomposeResult* out,
        Out Explanation* expl,
        In const ExplainOptions& opts) noexcept;

} // namespace matrix_core
