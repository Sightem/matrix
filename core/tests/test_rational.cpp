#include "matrix_core/rational.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

using matrix_core::ErrorCode;
using matrix_core::Rational;

static Rational make(std::int64_t n, std::int64_t d) {
		Rational r;
		assert(Rational::make(n, d, &r) == ErrorCode::Ok);
		return r;
}

int main() {
		const Rational normalized = make(2, -4);
		assert(normalized.num() == -1 && normalized.den() == 2);

		Rational out;
		assert(matrix_core::rational_add(make(1, 2), make(1, 3), &out) == ErrorCode::Ok);
		assert(out.num() == 5 && out.den() == 6);
		assert(matrix_core::rational_mul(make(3, 4), make(2, 3), &out) == ErrorCode::Ok);
		assert(out.num() == 1 && out.den() == 2);
		assert(matrix_core::rational_div(make(3, 4), make(2, 3), &out) == ErrorCode::Ok);
		assert(out.num() == 9 && out.den() == 8);

		constexpr auto min = std::numeric_limits<std::int64_t>::min();
		constexpr auto max = std::numeric_limits<std::int64_t>::max();
		assert(Rational::make(1, 0, &out) == ErrorCode::DivisionByZero);
		assert(matrix_core::rational_div(make(1, 2), make(0, 1), &out) == ErrorCode::DivisionByZero);
		assert(Rational::make(min, -1, &out) == ErrorCode::Overflow);
		assert(matrix_core::rational_neg(make(min, 1), &out) == ErrorCode::Overflow);
		assert(matrix_core::rational_sub(make(0, 1), make(min, 1), &out) == ErrorCode::Overflow);
		assert(matrix_core::rational_add(make(max, 1), make(1, 1), &out) == ErrorCode::Overflow);
		assert(matrix_core::rational_mul(make(max, 1), make(2, 1), &out) == ErrorCode::Overflow);
		return 0;
}
