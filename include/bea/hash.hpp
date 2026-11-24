#pragma once

#include <string>
#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	class Hash {
	public:
		Hash() {}

		Hash(
			const std::uint8_t *data,
			std::size_t size
		) {
			calculateValue(data, size);
		}

		Hash(
			const std::string& string
		) {
			calculateValue(
				(const std::uint8_t *)string.c_str(),
				string.length()
			);
		}

		Hash(const Name& name) {

			calculateValueName(name);
		}
		Hash(const TypeID& typeID) {

			calculateValueTypeID(typeID);
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
		void calculateValueName(const Name& name);
		void calculateValueTypeID(const TypeID& typeID);
	};

	std::ostream& operator<<(std::ostream& stream, const Hash& hash);
};
