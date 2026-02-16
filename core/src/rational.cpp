#include "matrix_core/rational.hpp"

#include <limits>

namespace matrix_core {
namespace {
constexpr std::uint64_t uabs(std::int64_t x) noexcept {
		// works for INT64_MIN
		return x < 0 ? (static_cast<std::uint64_t>(-(x + 1)) + 1u) : static_cast<std::uint64_t>(x);
}

constexpr std::uint64_t gcd_u64(std::uint64_t a, std::uint64_t b) noexcept {
		while (b != 0u) {
				std::uint64_t t = a % b;
				a = b;
				b = t;
		}
		return a;
}

ErrorCode normalize(std::int64_t* num, std::int64_t* den) noexcept {
		if (!num || !den)
				return ErrorCode::Internal;

		if (*den == 0)
				return ErrorCode::DivisionByZero;

		if (*num == 0) {
				*den = 1;
				return ErrorCode::Ok;
		}

		if (*den < 0) {
				if (*den == std::numeric_limits<std::int64_t>::min())
						return ErrorCode::Overflow;
				if (*num == std::numeric_limits<std::int64_t>::min())
						return ErrorCode::Overflow;
				*num = -*num;
				*den = -*den;
		}

		std::uint64_t g = gcd_u64(uabs(*num), static_cast<std::uint64_t>(*den));
		if (g > 1u) {
				*num /= static_cast<std::int64_t>(g);
				*den /= static_cast<std::int64_t>(g);
		}
		return ErrorCode::Ok;
}

template <typename T> bool add_overflow(T a, T b, T* out) noexcept {
		// false positive overflow results were observed on the CE toolchain when
		// using __builtin_add_overflow for signed 64 bit arithmetic (notably when
		// adding a negative term) which breaks algorithms like REF/RREF that rely
		// heavily on cancellation
		//
		// __int128 is also not supported on the CE target
		// TODO: revisit this with nightly toolchain
		if (!out)
				return true;

		constexpr T kMax = std::numeric_limits<T>::max();
		constexpr T kMin = std::numeric_limits<T>::min();

		if (b > 0) {
				if (a > static_cast<T>(kMax - b))
						return true;
		} else if (b < 0) {
				if (a < static_cast<T>(kMin - b))
						return true;
		}

		*out = static_cast<T>(a + b);
		return false;
}

template <typename T> bool sub_overflow(T a, T b, T* out) noexcept {
		if (!out)
				return true;

		constexpr T kMax = std::numeric_limits<T>::max();
		constexpr T kMin = std::numeric_limits<T>::min();

		if (b > 0) {
				if (a < static_cast<T>(kMin + b))
						return true;
		} else if (b < 0) {
				if (a > static_cast<T>(kMax + b))
						return true;
		}

		*out = static_cast<T>(a - b);
		return false;
}

template <typename T> bool mul_overflow(T a, T b, T* out) noexcept {
		if (!out)
				return true;

		constexpr T kMax = std::numeric_limits<T>::max();
		constexpr T kMin = std::numeric_limits<T>::min();

		if (a == 0 || b == 0) {
				*out = 0;
				return false;
		}

		if (a == 1) {
				*out = b;
				return false;
		}
		if (b == 1) {
				*out = a;
				return false;
		}

		// division overflow special case: min / -1
		if (a == -1) {
				if (b == kMin)
						return true;
				*out = static_cast<T>(-b);
				return false;
		}
		if (b == -1) {
				if (a == kMin)
						return true;
				*out = static_cast<T>(-a);
				return false;
		}

		// Safe multiplication overflow check (no __int128, no builtins).
		if (a > 0) {
				if (b > 0) {
						if (a > static_cast<T>(kMax / b))
								return true;
				} else {
						if (b < static_cast<T>(kMin / a))
								return true;
				}
		} else {
				if (b > 0) {
						if (a < static_cast<T>(kMin / b))
								return true;
				} else {
						// both negative -> positive
						if (b < static_cast<T>(kMax / a))
								return true;
				}
		}

		*out = static_cast<T>(a * b);
		return false;
}

} // namespace

ErrorCode Rational::make(std::int64_t num, std::int64_t den, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;

		ErrorCode ec = normalize(&num, &den);
		if (!is_ok(ec))
				return ec;

		*out = Rational(num, den);
		return ErrorCode::Ok;
}

ErrorCode rational_neg(const Rational& a, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;

		// Note: negating INT64_MIN overflows in two's complement.
		if (a.num() == std::numeric_limits<std::int64_t>::min())
				return ErrorCode::Overflow;

		const std::int64_t n = -a.num();
		const std::int64_t d = a.den();
		return Rational::make(n, d, out);
}

ErrorCode rational_add(const Rational& a, const Rational& b, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;

		// a/b + c/d = (a*(d/g) + c*(b/g)) / (b/g*d), where g=gcd(b,d)
		std::uint64_t g = gcd_u64(static_cast<std::uint64_t>(a.den()), static_cast<std::uint64_t>(b.den()));
		std::int64_t a_den_div_g = a.den() / static_cast<std::int64_t>(g);
		std::int64_t b_den_div_g = b.den() / static_cast<std::int64_t>(g);

		std::int64_t term1 = 0;
		if (mul_overflow(a.num(), b_den_div_g, &term1))
				return ErrorCode::Overflow;

		std::int64_t term2 = 0;
		if (mul_overflow(b.num(), a_den_div_g, &term2))
				return ErrorCode::Overflow;

		std::int64_t num = 0;
		if (add_overflow(term1, term2, &num))
				return ErrorCode::Overflow;

		std::int64_t den = 0;
		if (mul_overflow(a_den_div_g, b.den(), &den))
				return ErrorCode::Overflow;

		return Rational::make(num, den, out);
}

ErrorCode rational_sub(const Rational& a, const Rational& b, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;

		// important: do not implement subtraction as add(a, -b) on this target
		// CE toolchain overflow builtin oddities when adding negative
		// terms (even for small magnitudes). Computing the subtraction directly keeps
		// the overflow checks well-behaved and also avoids the INT64_MIN negation
		// edge case
		//
		// a/b - c/d = (a*(d/g) - c*(b/g)) / (b/g*d), where g=gcd(b,d)
		std::uint64_t g = gcd_u64(static_cast<std::uint64_t>(a.den()), static_cast<std::uint64_t>(b.den()));
		std::int64_t a_den_div_g = a.den() / static_cast<std::int64_t>(g);
		std::int64_t b_den_div_g = b.den() / static_cast<std::int64_t>(g);

		std::int64_t term1 = 0;
		if (mul_overflow(a.num(), b_den_div_g, &term1))
				return ErrorCode::Overflow;

		std::int64_t term2 = 0;
		if (mul_overflow(b.num(), a_den_div_g, &term2))
				return ErrorCode::Overflow;

		std::int64_t num = 0;
		if (sub_overflow(term1, term2, &num))
				return ErrorCode::Overflow;

		std::int64_t den = 0;
		if (mul_overflow(a_den_div_g, b.den(), &den))
				return ErrorCode::Overflow;

		return Rational::make(num, den, out);
}

ErrorCode rational_mul(const Rational& a, const Rational& b, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;

		// reduce cross terms to avoid overflow, (a/b)*(c/d)
		std::uint64_t g1 = gcd_u64(uabs(a.num()), static_cast<std::uint64_t>(b.den()));
		std::uint64_t g2 = gcd_u64(uabs(b.num()), static_cast<std::uint64_t>(a.den()));

		std::int64_t a_num_red = a.num() / static_cast<std::int64_t>(g1);
		std::int64_t b_den_red = b.den() / static_cast<std::int64_t>(g1);

		std::int64_t b_num_red = b.num() / static_cast<std::int64_t>(g2);
		std::int64_t a_den_red = a.den() / static_cast<std::int64_t>(g2);

		std::int64_t num = 0;
		if (mul_overflow(a_num_red, b_num_red, &num))
				return ErrorCode::Overflow;

		std::int64_t den = 0;
		if (mul_overflow(a_den_red, b_den_red, &den))
				return ErrorCode::Overflow;

		return Rational::make(num, den, out);
}

ErrorCode rational_div(const Rational& a, const Rational& b, Rational* out) noexcept {
		if (!out)
				return ErrorCode::Internal;
		if (b.is_zero())
				return ErrorCode::DivisionByZero;

		// a/b div c/d = (a/b) * (d/c)
		Rational recip;
		ErrorCode ec = Rational::make(b.den(), b.num(), &recip);
		if (!is_ok(ec))
				return ec;
		return rational_mul(a, recip, out);
}

} // namespace matrix_core
