#include "matrix_core/matrix_core.hpp"

#include "test_dbg_ce.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(MATRIX_CE_TESTS)
#include <debug.h>
#endif

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

static MatrixMutView mat4(Arena& a) {
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(a, 4, 4, &m) == ErrorCode::Ok);
		return m;
}

int main() {
#if defined(MATRIX_CE_TESTS)
#ifdef NDEBUG
		dbg_printf("[test_cofactor_element] NDEBUG defined (asserts off)\n");
#else
		dbg_printf("[test_cofactor_element] NDEBUG not defined (asserts on)\n");
#endif
#endif
		Slab slab;
		assert(slab.init(256 * 1024) == ErrorCode::Ok);

		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		// A =
		// [ 4 -1  1  6 ]
		// [ 0  0 -3  3 ]
		// [ 4  1  0 14 ]
		// [ 4  1  3  2 ]
		MatrixMutView a = mat4(persist);
		const std::int64_t avals[4][4] = {
		        {4, -1, 1, 6},
		        {0, 0, -3, 3},
		        {4, 1, 0, 14},
		        {4, 1, 3, 2},
		};
		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++)
						a.at_mut(r, c) = Rational::from_int(avals[r][c]);
		}

		const std::int64_t expected_cof[4][4] = {
		        {-27, 108, 0, 0},
		        {-72, -48, 96, 24},
		        {36, -24, -24, -24},
		        {-63, -84, 24, 24},
		};

		for (std::uint8_t r = 0; r < 4; r++) {
				for (std::uint8_t c = 0; c < 4; c++) {
						Rational cof;
						auto err = matrix_core::op_cofactor_element(
						        a.view(), r, c, scratch, &cof, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
						assert(matrix_core::is_ok(err));

						assert(cof.den() == 1);
						assert(cof.num() == expected_cof[r][c]);

#if defined(MATRIX_CE_TESTS)
						if ((r == 0 && c == 0) || (r == 1 && c == 0) || (r == 3 && c == 3)) {
								static char label[32];
								matrix_core::Writer w{label, sizeof(label), 0};
								(void)w.append("C(");
								(void)w.append_u64(r);
								(void)w.append(",");
								(void)w.append_u64(c);
								(void)w.append(")");
								matrix_test_ce::print_rational(label, cof);
						}
#endif
				}
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_cofactor_element] after expected cofactors asserts\n");
#endif

		// 1x1: cofactor=1.
		{
				MatrixMutView one;
				assert(matrix_core::matrix_alloc(persist, 1, 1, &one) == ErrorCode::Ok);
				one.at_mut(0, 0) = Rational::from_int(7);

				Rational cof;
				Explanation expl;
				auto err = matrix_core::op_cofactor_element(
				        one.view(), 0, 0, scratch, &cof, &expl, ExplainOptions{.enable = true, .persist = &persist});
				assert(matrix_core::is_ok(err));
				assert(cof.num() == 1 && cof.den() == 1);
				assert(expl.available());
				assert(expl.step_count() == 2);

				char caption[64];
				char latex[128];
				StepRenderBuffers bufs{caption, sizeof(caption), latex, sizeof(latex), &scratch};
				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(expl.render_step(1, bufs) == ErrorCode::Ok);
				assert(std::strcmp(latex, "$$C_{1,1} = (-1)^{2} M_{1,1} = 1$$") == 0);

#if defined(MATRIX_CE_TESTS)
				dbg_printf("%s\n", latex);
#endif
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_cofactor_element] after 1x1 asserts\n");
#endif

		// Overflow during cofactor sign flip (minor == INT64_MIN).
		{
				MatrixMutView m;
				assert(matrix_core::matrix_alloc(persist, 2, 2, &m) == ErrorCode::Ok);
				matrix_core::matrix_fill_zero(m);
				m.at_mut(1, 0) = Rational::from_int(std::numeric_limits<std::int64_t>::min());

				Rational cof;
				auto err = matrix_core::op_cofactor_element(
				        m.view(), 0, 1, scratch, &cof, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::Overflow);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_cofactor_element] after overflow asserts\n");
#endif

		// Argument validation.
		{
				Rational cof;
				auto err = matrix_core::op_cofactor_element(
				        a.view(), 4, 0, scratch, &cof, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::IndexOutOfRange);
				assert(err.i == 4);

				MatrixMutView ns;
				assert(matrix_core::matrix_alloc(persist, 2, 3, &ns) == ErrorCode::Ok);
				err = matrix_core::op_cofactor_element(
				        ns.view(), 0, 0, scratch, &cof, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::NotSquare);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_cofactor_element] after validation asserts\n");
#endif

		// Spot-check step breakdown for (i,j)=(2,1) in 1-based math, i.e. (1,0) in 0-based.
		{
				Rational cof;
				Explanation expl;
				auto err = matrix_core::op_cofactor_element(
				        a.view(), 1, 0, scratch, &cof, &expl, ExplainOptions{.enable = true, .persist = &persist});
				assert(matrix_core::is_ok(err));
				assert(cof.num() == -72 && cof.den() == 1);
				assert(expl.available());

				const std::size_t nsteps = expl.step_count();
				assert(nsteps >= 3);

				char caption[128];
				char latex[512];
				StepRenderBuffers bufs{caption, sizeof(caption), latex, sizeof(latex), &scratch};

				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);

				assert(expl.render_step(1, bufs) == ErrorCode::Ok);
				assert(std::strstr(caption, "Delete row") != nullptr);

				char tiny_caption[8];
				StepRenderBuffers tiny_cap{tiny_caption, sizeof(tiny_caption), latex, sizeof(latex), &scratch};
				assert(expl.render_step(1, tiny_cap) == ErrorCode::BufferTooSmall);

				// If we have row-ops, render at least one elimination step.
				if (nsteps > 3) {
						assert(expl.render_step(2, bufs) == ErrorCode::Ok);
						assert(std::strlen(caption) > 0);
				}

				assert(expl.render_step(nsteps - 1, bufs) == ErrorCode::Ok);
				assert(std::strcmp(latex, "$$C_{2,1} = (-1)^{3} M_{2,1} = -72$$") == 0);

#if defined(MATRIX_CE_TESTS)
				dbg_printf("%s\n", latex);
#endif

				for (std::size_t i = 0; i < nsteps; i++)
						assert(expl.render_step(i, bufs) == ErrorCode::Ok);

				assert(expl.render_step(nsteps, bufs) == ErrorCode::StepOutOfRange);

				StepRenderBuffers no_scratch{caption, sizeof(caption), latex, sizeof(latex), nullptr};
				assert(expl.render_step(0, no_scratch) == ErrorCode::Internal);

				char tiny_latex[8];
				StepRenderBuffers tiny{caption, sizeof(caption), tiny_latex, sizeof(tiny_latex), &scratch};
				assert(expl.render_step(nsteps - 1, tiny) == ErrorCode::BufferTooSmall);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_cofactor_element] after steps asserts\n");
#endif

		return 0;
}
