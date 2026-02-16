#include "matrix_core/ops.hpp"

#if !MATRIX_CORE_ENABLE_CRAMER
namespace matrix_core {

Error op_det_replace_column(MatrixView a,
        MatrixView b,
        std::uint8_t col,
        Arena& scratch,
        Rational* out,
        Explanation* expl,
        const ExplainOptions& opts) noexcept {
		(void)a;
		(void)b;
		(void)col;
		(void)scratch;
		(void)out;
		(void)expl;
		(void)opts;
		return err_feature_disabled();
}

Error op_cramer_solve(MatrixView a, MatrixView b, Arena& scratch, MatrixMutView x_out) noexcept {
		(void)a;
		(void)b;
		(void)scratch;
		(void)x_out;
		return err_feature_disabled();
}

} // namespace matrix_core
#endif // !MATRIX_CORE_ENABLE_CRAMER

#if !MATRIX_CORE_ENABLE_COFACTOR
namespace matrix_core {

Error op_cofactor_element(MatrixView a,
        std::uint8_t i,
        std::uint8_t j,
        Arena& scratch,
        Rational* out,
        Explanation* expl,
        const ExplainOptions& opts) noexcept {
		(void)a;
		(void)i;
		(void)j;
		(void)scratch;
		(void)out;
		(void)expl;
		(void)opts;
		return err_feature_disabled();
}

} // namespace matrix_core
#endif // !MATRIX_CORE_ENABLE_COFACTOR

#if !MATRIX_CORE_ENABLE_MINOR_MATRIX
namespace matrix_core {

Error op_minor_matrix(MatrixView a, Arena& scratch, MatrixMutView out) noexcept {
		(void)a;
		(void)scratch;
		(void)out;
		return err_feature_disabled();
}

} // namespace matrix_core
#endif // !MATRIX_CORE_ENABLE_MINOR_MATRIX

