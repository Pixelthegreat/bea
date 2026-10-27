#include <bea/core.hpp>
#include <bea/hash.hpp>
namespace bea {

	/* Raise to power */
	static std::uint32_t hashPower(std::uint32_t x, std::uint32_t y) {

		std::uint32_t result = 1;
		for (std::uint32_t i = 0; i < y; i++)
			result *= x;
		return result;
	}

	/* Calculate hash */
	void Hash::calculateValue(
		const std::uint8_t *data,
		std::size_t size
	) {
		m_value = 0;
		if (!size) return;

		for (std::uint32_t i = 0; i < size-1; i++)
			m_value +=
				(std::uint32_t)data[i] *
				hashPower(31, (std::uint32_t)size-i-1);

		m_value += (std::uint32_t)data[size-1];
	}

	/* Convert hash to string */
	std::string Hash::toString() const {

		std::string string(8, ' ');
		const char *hex = "0123456789abcdef";

		for (std::size_t i = 0; i < 8; i++)
			string[7-i] = hex[(m_value >> (4 * i)) & 0xf];

		return string;
	}

	/* Print hash value */
	std::ostream& operator<<(std::ostream& stream, const Hash& hash) {

		return stream << hash.toString();
	}
}
