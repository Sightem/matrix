#include "matrix_core/matrix_core.hpp"

#include "test_dbg_ce.hpp"

#include <cassert>
#include <cstring>

#if defined(MATRIX_CE_TESTS)
#include <debug.h>
#endif

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::ProjDecomposeResult;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

static MatrixMutView col_vec3(Arena& a, std::int64_t x1, std::int64_t x2, std::int64_t x3) {
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(a, 3, 1, &m) == ErrorCode::Ok);
		m.at_mut(0, 0) = Rational::from_int(x1);
		m.at_mut(1, 0) = Rational::from_int(x2);
		m.at_mut(2, 0) = Rational::from_int(x3);
		return m;
}

static MatrixMutView row_vec3(Arena& a, std::int64_t x1, std::int64_t x2, std::int64_t x3) {
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(a, 1, 3, &m) == ErrorCode::Ok);
		m.at_mut(0, 0) = Rational::from_int(x1);
		m.at_mut(0, 1) = Rational::from_int(x2);
		m.at_mut(0, 2) = Rational::from_int(x3);
		return m;
}

int main() {
#if defined(MATRIX_CE_TESTS)
#ifdef NDEBUG
		dbg_printf("[test_vectors] NDEBUG defined (asserts off)\n");
#else
		dbg_printf("[test_vectors] NDEBUG not defined (asserts on)\n");
#endif
#endif
		Slab slab;
		assert(slab.init(128 * 1024) == ErrorCode::Ok);

		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		// Dot product.
		{
				MatrixMutView u = col_vec3(persist, 1, 2, 3);
				MatrixMutView v = row_vec3(persist, 4, 5, 6);
				Rational out;
				Explanation expl;
				auto err = matrix_core::op_dot(u.view(), v.view(), &out, &expl, ExplainOptions{.enable = true, .persist = &persist});
				assert(matrix_core::is_ok(err));
				assert(out.den() == 1 && out.num() == 32);
				assert(expl.available());
				assert(expl.step_count() == 1);

#if defined(MATRIX_CE_TESTS)
				static char latex[512];
#else
				char latex[512];
#endif
				StepRenderBuffers bufs{nullptr, 0, latex, sizeof(latex), &scratch};
				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "u\\cdot v") != nullptr);
				assert(std::strstr(latex, "32") != nullptr);

#if defined(MATRIX_CE_TESTS)
				matrix_test_ce::print_rational("u·v", out);
				dbg_printf("%s\n", latex);
#endif
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_vectors] after dot asserts\n");
#endif

		// Dot dimension mismatch.
		{
				MatrixMutView u;
				assert(matrix_core::matrix_alloc(persist, 2, 1, &u) == ErrorCode::Ok);
				MatrixMutView v;
				assert(matrix_core::matrix_alloc(persist, 3, 1, &v) == ErrorCode::Ok);
				Rational out;
				auto err = matrix_core::op_dot(u.view(), v.view(), &out, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::DimensionMismatch);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_vectors] after dot mismatch asserts\n");
#endif

		// Cross product (3D).
		{
				MatrixMutView u = col_vec3(persist, 1, 2, 3);
				MatrixMutView v = col_vec3(persist, 4, 5, 6);
				MatrixMutView out;
				assert(matrix_core::matrix_alloc(persist, 3, 1, &out) == ErrorCode::Ok);

				Explanation expl;
				auto err = matrix_core::op_cross(u.view(), v.view(), out, &expl, ExplainOptions{.enable = true, .persist = &persist});
				assert(matrix_core::is_ok(err));
				assert(out.at(0, 0).den() == 1 && out.at(0, 0).num() == -3);
				assert(out.at(1, 0).den() == 1 && out.at(1, 0).num() == 6);
				assert(out.at(2, 0).den() == 1 && out.at(2, 0).num() == -3);

#if defined(MATRIX_CE_TESTS)
				matrix_test_ce::print_matrix("u×v", out.view());
#endif

#if defined(MATRIX_CE_TESTS)
				static char caption[64];
				static char latex[512];
#else
				char caption[64];
				char latex[512];
#endif
				StepRenderBuffers bufs{caption, sizeof(caption), latex, sizeof(latex), &scratch};
				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "u\\times v") != nullptr);
				assert(expl.render_step(1, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_vectors] after cross asserts\n");
#endif

		// Projection decomposition: u onto v.
#if MATRIX_CORE_ENABLE_PROJECTION
		{
				MatrixMutView u = col_vec3(persist, 1, 2, 3);
				MatrixMutView v = col_vec3(persist, 4, 5, 6);
				MatrixMutView proj;
				MatrixMutView orth;
				assert(matrix_core::matrix_alloc(persist, 3, 1, &proj) == ErrorCode::Ok);
				assert(matrix_core::matrix_alloc(persist, 3, 1, &orth) == ErrorCode::Ok);

				ProjDecomposeResult r;
				Explanation expl;
				auto err = matrix_core::op_proj_decompose_u_onto_v(
				        u.view(), v.view(), proj, orth, &r, &expl, ExplainOptions{.enable = true, .persist = &persist});
				assert(matrix_core::is_ok(err));

				// dot_uv=32, dot_vv=77, k=32/77
				assert(r.dot_uv.den() == 1 && r.dot_uv.num() == 32);
				assert(r.dot_vv.den() == 1 && r.dot_vv.num() == 77);
				assert(r.k.num() == 32 && r.k.den() == 77);

				// proj = (32/77) * [4,5,6] = [128/77,160/77,192/77]
				assert(proj.at(0, 0).num() == 128 && proj.at(0, 0).den() == 77);
				assert(proj.at(1, 0).num() == 160 && proj.at(1, 0).den() == 77);
				assert(proj.at(2, 0).num() == 192 && proj.at(2, 0).den() == 77);

				// orth = u - proj = [-51/77, -6/77, 39/77]
				assert(orth.at(0, 0).num() == -51 && orth.at(0, 0).den() == 77);
				assert(orth.at(1, 0).num() == -6 && orth.at(1, 0).den() == 77);
				assert(orth.at(2, 0).num() == 39 && orth.at(2, 0).den() == 77);

				// ||proj||^2 = 1024/77, ||orth||^2 = 54/77
				assert(r.proj_norm2.num() == 1024 && r.proj_norm2.den() == 77);
				assert(r.orth_norm2.num() == 54 && r.orth_norm2.den() == 77);

#if defined(MATRIX_CE_TESTS)
				matrix_test_ce::print_rational("k", r.k);
				matrix_test_ce::print_matrix("proj", proj.view());
				matrix_test_ce::print_matrix("orth", orth.view());
#endif

				assert(expl.available());
				assert(expl.step_count() == 8);

#if defined(MATRIX_CE_TESTS)
				static char caption[64];
				static char latex[1024];
#else
				char caption[64];
				char latex[1024];
#endif
				StepRenderBuffers bufs{caption, sizeof(caption), latex, sizeof(latex), &scratch};

				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "proj_v(u)") != nullptr);
				assert(expl.render_step(4, bufs) == ErrorCode::Ok);
				assert(std::strstr(caption, "proj") != nullptr);
				assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);
				assert(expl.render_step(7, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "||orth||^2") != nullptr);

				for (std::size_t i = 0; i < expl.step_count(); i++)
						assert(expl.render_step(i, bufs) == ErrorCode::Ok);
				assert(expl.render_step(expl.step_count(), bufs) == ErrorCode::StepOutOfRange);
		}

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_vectors] after projection asserts\n");
#endif

		// Projection division-by-zero (v is zero vector).
		{
				MatrixMutView u = col_vec3(persist, 1, 2, 3);
				MatrixMutView v = col_vec3(persist, 0, 0, 0);
				MatrixMutView proj;
				MatrixMutView orth;
				assert(matrix_core::matrix_alloc(persist, 3, 1, &proj) == ErrorCode::Ok);
				assert(matrix_core::matrix_alloc(persist, 3, 1, &orth) == ErrorCode::Ok);

				ProjDecomposeResult r;
				auto err = matrix_core::op_proj_decompose_u_onto_v(
				        u.view(), v.view(), proj, orth, &r, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::DivisionByZero);
		}
#else
		{
				MatrixMutView u = col_vec3(persist, 1, 2, 3);
				MatrixMutView v = col_vec3(persist, 4, 5, 6);
				MatrixMutView proj;
				MatrixMutView orth;
				assert(matrix_core::matrix_alloc(persist, 3, 1, &proj) == ErrorCode::Ok);
				assert(matrix_core::matrix_alloc(persist, 3, 1, &orth) == ErrorCode::Ok);

				ProjDecomposeResult r;
				auto err = matrix_core::op_proj_decompose_u_onto_v(
				        u.view(), v.view(), proj, orth, &r, nullptr, ExplainOptions{.enable = false, .persist = nullptr});
				assert(!matrix_core::is_ok(err));
				assert(err.code == ErrorCode::FeatureDisabled);
		}
#endif

#if defined(MATRIX_CE_TESTS)
		dbg_printf("[test_vectors] after proj div-by-zero asserts\n");
#endif

		return 0;
}
