#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"

namespace matrix_core {

struct Writer {
		char* data;
		std::size_t cap;
		std::size_t len;

		explicit Writer(char* buffer = nullptr, std::size_t capacity = 0, std::size_t initial_len = 0) noexcept
		    : data(buffer), cap(capacity), len(initial_len) {
				if (!data || cap == 0) {
						error_ = ErrorCode::BufferTooSmall;
						return;
				}
				if (len >= cap) {
						len = cap - 1;
						error_ = ErrorCode::BufferTooSmall;
				}
				data[len] = '\0';
		}

		ErrorCode status() const noexcept { return error_; }

		void put(char ch) noexcept {
				if (!is_ok(error_))
						return;
				if (len >= cap - 1) {
						error_ = ErrorCode::BufferTooSmall;
						return;
				}
				data[len++] = ch;
				data[len] = '\0';
		}

		void append(const char* s) noexcept {
				if (!is_ok(error_))
						return;
				if (!s) {
						error_ = ErrorCode::Internal;
						return;
				}
				for (std::size_t i = 0; is_ok(error_) && s[i] != '\0'; i++)
						put(s[i]);
		}

		void append_u64(std::uint64_t v) noexcept {
				if (!is_ok(error_))
						return;
				char buf[32];
				std::size_t n = 0;
				do {
						buf[n++] = static_cast<char>('0' + (v % 10u));
						v /= 10u;
				} while (v != 0u);

				for (std::size_t i = 0; i < n && is_ok(error_); i++)
						put(buf[n - 1 - i]);
		}

		void append_i64(std::int64_t v) noexcept {
				if (!is_ok(error_))
						return;
				if (v < 0)
						put('-');
				if (!is_ok(error_))
						return;
				const std::uint64_t mag =
				        (v < 0) ? (static_cast<std::uint64_t>(-(v + 1)) + 1u) : static_cast<std::uint64_t>(v);
				append_u64(mag);
		}

		// append a 1 based index (v+1) as decimal
		void append_index1(std::uint8_t v) noexcept {
				if (is_ok(error_))
						append_u64(static_cast<std::uint64_t>(v) + 1u);
		}

		// append a rational as LaTeX (either integer or \frac{num}{den})
		void append_rational_latex(const Rational& r) noexcept {
				if (!is_ok(error_))
						return;
				if (r.den() == 1) {
						append_i64(r.num());
						return;
				}
				append("\\frac{");
				append_i64(r.num());
				append("}{");
				append_i64(r.den());
				append("}");
		}

	  private:
		ErrorCode error_ = ErrorCode::Ok;
};

// Writer wrapper that fail-fast aborts on the first error.
// Intended for cases where overflows indicate a programmer error.
struct CheckedWriter final {
		explicit CheckedWriter(char* data, std::size_t cap) noexcept : w{data, cap, 0} { check(w.status()); }

		void put(char ch) noexcept {
				w.put(ch);
				check(w.status());
		}
		void append(const char* s) noexcept {
				w.append(s);
				check(w.status());
		}
		void append_u64(std::uint64_t v) noexcept {
				w.append_u64(v);
				check(w.status());
		}
		void append_i64(std::int64_t v) noexcept {
				w.append_i64(v);
				check(w.status());
		}
		void append_index1(std::uint8_t v) noexcept {
				w.append_index1(v);
				check(w.status());
		}
		void append_rational_latex(const Rational& r) noexcept {
				w.append_rational_latex(r);
				check(w.status());
		}

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
