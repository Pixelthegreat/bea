#pragma once

#include <fstream>
#include <bea/type.hpp>

namespace bea {

	/*
	 * Serializer
	 */
	class Serializer {
	public:
		Serializer(std::ostream& stream)
			: m_stream(stream) {}

		/*
		 * Write bytes
		 */
		void writeU8(std::uint8_t value);
		void writeU16L(std::uint16_t value);
		void writeU32L(std::uint32_t value);
		void writeU64L(std::uint64_t value);

		/*
		 * Write string
		 */
		void writeString(const std::string& string);

		/*
		 * Write name
		 */
		void writeName(const Name& name);
	private:
		std::ostream& m_stream;
	};

	/*
	 * Deserializer
	 */
	class Deserializer {
	public:
		Deserializer(std::istream& stream)
			: m_stream(stream) {}

		/*
		 * Read bytes
		 */
		std::uint8_t readU8();
		std::uint16_t readU16L();
		std::uint32_t readU32L();
		std::uint64_t readU64L();

		/*
		 * Read string
		 */
		std::string readString();

		/*
		 * Read name
		 */
		Name readName();
	private:
		std::istream& m_stream;
	};
};
