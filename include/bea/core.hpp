#pragma once

#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <string_view>
#include <memory>
#include <array>
#include <vector>
#include <list>
#include <utility>

namespace bea {

	/*
	 * Common types and values
	 */
	using Int = int64_t;
	using UInt = uint64_t;
	using Boolean = uint8_t;
	using None = uint8_t;

	/*
	 * Error position
	 */
	struct Position {
		const char *path = nullptr;
		long line = 0;
		long column = 0;
	};

	std::ostream& operator<<(std::ostream& stream, const Position& position);

	/*
	 * Error type
	 */
	struct Error {
		Position position{};
		std::string message{};

		Error(std::string message) : message(message), position() {}

		Error(Position position, std::string message)
			: position(position), message(message) {}
	};

	std::ostream& operator<<(std::ostream& stream, const Error& error);

	/*
	 * Result type
	 */
	template <typename T>
	struct Result {
		bool okay;
		union {
			T value;
			std::unique_ptr<Error> error = nullptr;
		};

		Result() : okay(true) {}

		Result(T value) : value(std::move(value)), okay(true) {}

		Result(Result<T>&& other) : okay(other.okay) {

			if (okay) value = std::move(other.value);
			else error = std::move(other.error);
		}

		void operator=(Result<T>&& other) {

			okay = other.okay;

			if (okay) value = std::move(other.value);
			else error = std::move(other.error);
		}

		~Result() {}

		Result(std::unique_ptr<Error> error)
			: error(std::move(error)), okay(false) {}

		Result(std::string message)
			: error(std::make_unique<Error>(message)),
			  okay(false) {}

		Result(Position position, std::string message)
			: error(std::make_unique<Error>(position, message)),
			  okay(false) {}

		static Result<T> fail() {

			Result<T> result;
			result.okay = false;

			return result;
		}
	};

	/*
	 * Is instance
	 */
	template <typename T, typename U>
	inline bool isInstance(U& value) {

		return !!dynamic_cast<T*>(&value);
	}
}

#define BEA_OK(T, p_result) ({\
		auto __result = (p_result);\
		if (!__result.okay) return ::bea::Result<T>(::std::move(__result.error));\
		std::move(__result.value);\
	})
