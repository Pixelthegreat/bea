#include <bea/core.hpp>
#include <bea/serial.hpp>

namespace bea {

	/*
	 * Write uint8
	 */
	void Serializer::writeU8(std::uint8_t value) {

		m_stream.put((char)value);
	}

	/*
	 * Write little endian uint16
	 */
	void Serializer::writeU16L(std::uint16_t value) {

		writeU8((std::uint8_t)(value & 0xff));
		writeU8((std::uint8_t)((value >> 8) & 0xff));
	}

	/*
	 * Write little endian uint32
	 */
	void Serializer::writeU32L(std::uint32_t value) {

		writeU8((std::uint8_t)(value & 0xff));
		writeU8((std::uint8_t)((value >> 8) & 0xff));
		writeU8((std::uint8_t)((value >> 16) & 0xff));
		writeU8((std::uint8_t)((value >> 24) & 0xff));
	}

	/*
	 * Write little endian uint64
	 */
	void Serializer::writeU64L(std::uint64_t value) {

		writeU8((std::uint8_t)(value & 0xff));
		writeU8((std::uint8_t)((value >> 8) & 0xff));
		writeU8((std::uint8_t)((value >> 16) & 0xff));
		writeU8((std::uint8_t)((value >> 24) & 0xff));
		writeU8((std::uint8_t)((value >> 32) & 0xff));
		writeU8((std::uint8_t)((value >> 40) & 0xff));
		writeU8((std::uint8_t)((value >> 48) & 0xff));
		writeU8((std::uint8_t)((value >> 56) & 0xff));
	}

	/*
	 * Write string
	 */
	void Serializer::writeString(const std::string& string) {

		writeU32L((std::uint32_t)string.length());

		for (std::size_t i = 0; i < string.length(); i++)
			writeU8((std::uint8_t)string[i]);
	}

	/*
	 * Write name
	 */
	void Serializer::writeName(const Name& name) {

		writeU8((std::uint8_t)name.components.size());

		for (std::size_t i = 0; i < name.components.size(); i++)
			writeString(name.components[i]);
	}

	/*
	 * Read uint8
	 */
	std::uint8_t Deserializer::readU8() {

		return (std::uint8_t)m_stream.get();
	}

	/*
	 * Read little endian uint16
	 */
	std::uint16_t Deserializer::readU16L() {

		return  (std::uint16_t)readU8() |
			((std::uint16_t)readU8() << 8);
	}

	/*
	 * Read little endian uint32
	 */
	std::uint32_t Deserializer::readU32L() {

		return  (std::uint32_t)readU8() |
			((std::uint32_t)readU8() << 8) |
			((std::uint32_t)readU8() << 16) |
			((std::uint32_t)readU8() << 24);
	}

	/*
	 * Read little endian uint64
	 */
	std::uint64_t Deserializer::readU64L() {

		return  (std::uint64_t)readU8() |
			((std::uint64_t)readU8() << 8) |
			((std::uint64_t)readU8() << 16) |
			((std::uint64_t)readU8() << 24) |
			((std::uint64_t)readU8() << 32) |
			((std::uint64_t)readU8() << 40) |
			((std::uint64_t)readU8() << 48) |
			((std::uint64_t)readU8() << 56);
	}

	/*
	 * Read string
	 */
	std::string Deserializer::readString() {

		std::string string(
			' ', (std::size_t)readU32L()
		);
		for (std::size_t i = 0; i < string.length(); i++)
			string[i] = (char)readU8();

		return std::move(string);
	}

	/*
	 * Read name
	 */
	Name Deserializer::readName() {

		Name name((std::size_t)readU8());

		for (std::size_t i = 0; i < name.components.size(); i++)
			name.components[i] = readString();

		return std::move(name);
	}
}
