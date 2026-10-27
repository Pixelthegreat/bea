#pragma once

#include <string>
#include <bea/core.hpp>

namespace bea {

	class Hash {
	public:
		Hash(
			const std::uint8_t *data,
			std::size_t size
		) {
			calculateValue(data, size);
		}

		std::uint32_t getValue() const { return m_value; }

		/*
		 * Convert hash to string
		 */
		std::string toString() const;
	private:
		std::uint32_t m_value;

		/*
		 * Calcualte hash
		 */
		void calculateValue(
			const std::uint8_t *data,
			std::size_t size
		);
	};

	std::ostream& operator<<(std::ostream& stream, const Hash& hash);
};
