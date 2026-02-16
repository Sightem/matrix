#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"

namespace matrix_core {

// string builder helper that writes to a fixed capacity buffer
// all operations are noexcept and return ErrorCode on overflow
struct Writer {
		char* data = nullptr;
		std::size_t cap = 0;
		std::size_t len = 0;

		ErrorCode put(char ch) noexcept {
				if (!data || cap == 0)
						return ErrorCode::BufferTooSmall;
				if (len + 1 >= cap)
						return ErrorCode::BufferTooSmall;
				data[len++] = ch;
				data[len] = '\0';
				return ErrorCode::Ok;
		}

		ErrorCode append(const char* s) noexcept {
				if (!s)
						return ErrorCode::Internal;
				for (std::size_t i = 0; s[i] != '\0'; i++) {
						ErrorCode ec = put(s[i]);
						if (!is_ok(ec))
								return ec;
				}
				return ErrorCode::Ok;
		}

		ErrorCode append_u64(std::uint64_t v) noexcept {
				char buf[32];
				std::size_t n = 0;
				do {
						buf[n++] = static_cast<char>('0' + (v % 10u));
						v /= 10u;
				} while (v != 0u);

				for (std::size_t i = 0; i < n; i++) {
						ErrorCode ec = put(buf[n - 1 - i]);
						if (!is_ok(ec))
								return ec;
				}
				return ErrorCode::Ok;
		}

		ErrorCode append_i64(std::int64_t v) noexcept {
				std::uint64_t mag = (v < 0) ? (static_cast<std::uint64_t>(-(v + 1)) + 1u) : static_cast<std::uint64_t>(v);
				if (v < 0) {
						ErrorCode ec = put('-');
						if (!is_ok(ec))
								return ec;
				}
				return append_u64(mag);
		}

		// append a 1 based index (v+1) as decimal
		ErrorCode append_index1(std::uint8_t v) noexcept { return append_u64(static_cast<std::uint64_t>(v) + 1u); }

		// append a rational as LaTeX (either integer or \frac{num}{den})
		ErrorCode append_rational_latex(const Rational& r) noexcept {
				if (r.den() == 1)
						return append_i64(r.num());

				ErrorCode ec = append("\\frac{");
				if (!is_ok(ec))
						return ec;
				ec = append_i64(r.num());
				if (!is_ok(ec))
						return ec;
				ec = append("}{");
				if (!is_ok(ec))
						return ec;
				ec = append_i64(r.den());
				if (!is_ok(ec))
						return ec;
				return append("}");
		}
};

// Writer wrapper that fail-fast aborts on the first error.
// Intended for cases where overflows indicate a programmer error.
struct CheckedWriter final {
		explicit CheckedWriter(char* data, std::size_t cap) noexcept : w{data, cap, 0} {}

		void put(char ch) noexcept { check(w.put(ch)); }
		void append(const char* s) noexcept { check(w.append(s)); }
		void append_u64(std::uint64_t v) noexcept { check(w.append_u64(v)); }
		void append_i64(std::int64_t v) noexcept { check(w.append_i64(v)); }
		void append_index1(std::uint8_t v) noexcept { check(w.append_index1(v)); }
		void append_rational_latex(const Rational& r) noexcept { check(w.append_rational_latex(r)); }

		Writer w{};

	  private:
		[[noreturn]] static void die(ErrorCode ec) noexcept {
				(void)ec;
				assert(false && "matrix_core::CheckedWriter failure");
				std::abort();
		}

		static void check(ErrorCode ec) noexcept {
				if (!is_ok(ec))
						die(ec);
		}
};

} // namespace matrix_core
