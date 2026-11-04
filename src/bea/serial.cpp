#include <bea/core.hpp>
#include <bea/serial.hpp>

namespace bea {

	/* Write uint8 */
	void Serializer::writeU8(std::uint8_t value) {

		m_stream.put((char)value);
	}

	/* Write little endian uint16 */
	void Serializer::writeU16L(std::uint16_t value) {

		writeU8((std::uint8_t)(value & 0xff));
		writeU8((std::uint8_t)((value >> 8) & 0xff));
	}

	/* Write little endian uint32 */
	void Serializer::writeU32L(std::uint32_t value) {

		writeU8((std::uint8_t)(value & 0xff));
		writeU8((std::uint8_t)((value >> 8) & 0xff));
		writeU8((std::uint8_t)((value >> 16) & 0xff));
		writeU8((std::uint8_t)((value >> 24) & 0xff));
	}

	/* Write little endian uint64 */
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

	/* Write string */
	void Serializer::writeString(const std::string& string) {

		writeU32L((std::uint32_t)string.length());

		for (std::size_t i = 0; i < string.length(); i++)
			writeU8((std::uint8_t)string[i]);
	}

	/* Write name */
	void Serializer::writeName(const Name& name) {

		writeU8((std::uint8_t)name.components.size());

		for (std::size_t i = 0; i < name.components.size(); i++)
			writeString(name.components[i]);
	}

	/* Read uint8 */
	std::uint8_t Deserializer::readU8() {

		return (std::uint8_t)m_stream.get();
	}

	/* Read little endian uint16 */
	std::uint16_t Deserializer::readU16L() {

		return  (std::uint16_t)readU8() |
			((std::uint16_t)readU8() << 8);
	}

	/* Read little endian uint32 */
	std::uint32_t Deserializer::readU32L() {

		return  (std::uint32_t)readU8() |
			((std::uint32_t)readU8() << 8) |
			((std::uint32_t)readU8() << 16) |
			((std::uint32_t)readU8() << 24);
	}

	/* Read little endian uint64 */
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

	/* Read string */
	std::string Deserializer::readString() {

		std::string string(
			' ', (std::size_t)readU32L()
		);
		for (std::size_t i = 0; i < string.length(); i++)
			string[i] = (char)readU8();

		return std::move(string);
	}

	/* Read name */
	Name Deserializer::readName() {

		Name name((std::size_t)readU8());

		for (std::size_t i = 0; i < name.components.size(); i++)
			name.components[i] = readString();

		return std::move(name);
	}

	/* Write GRCode::Use */
	void GRCodeSerializer::writeUse(
		const Name& name,
		const Name& alias
	) {
		writeU8((std::uint8_t)GRCode::Use);
		writeName(name);
		writeName(alias);
	}

	/* Write GRCode::Class */
	void GRCodeSerializer::writeClass(
		const Name& name,
		const Type& type
	) {
		writeU8((std::uint8_t)GRCode::Class);

		std::uint8_t flags = 0;
		if (type.object.flagAbstract)
			flags |= 1;
		writeU8(flags);

		writeName(name);

		writeU16L((std::uint16_t)type.object.genericParameters.size());
		writeU16L((std::uint16_t)type.object.publicMembers.size());
		writeU16L((std::uint16_t)type.object.privateMembers.size());
		writeU16L((std::uint16_t)type.object.implementations.size());

		for (const GenericInfo& generic: type.object.genericParameters)
			writeGeneric(generic, GRCode::GenericParameter);

		for (
			const std::pair<std::string, VariableInfo>& member:
			type.object.publicMembers
		)
			writeClassMember(member.first, member.second);

		for (
			const std::pair<std::string, VariableInfo>& member:
			type.object.privateMembers
		)
			writeClassMember(member.first, member.second);

		for (
			const ObjectImplementation& implementation:
			type.object.implementations
		)
			writeClassImpl(implementation);
	}

	/* Write GRCode::ClassField */
	void GRCodeSerializer::writeClassField(
		const Name& name,
		const VariableInfo& field
	) {
		writeU8((std::uint8_t)GRCode::ClassField);
		writeName(name);
		writeTypeID(*field.type);
	}

	/* Write GRCode::ClassImpl */
	void GRCodeSerializer::writeClassImpl(
		const ObjectImplementation& implementation
	) {
		writeU8((std::uint8_t)GRCode::ClassImpl);
		writeTypeID(*implementation.type);
		writeU16L((std::uint16_t)implementation.members.size());

		for (
			const std::pair<std::string, VariableInfo>& member:
			implementation.members
		)
			writeClassMember(member.first, member.second);
	}

	/* Write GRCode::Function */
	void GRCodeSerializer::writeFunction(
		const Name& name,
		const VariableInfo& variable
	) {
		Type& type = *variable.type->type;

		writeU8((std::uint8_t)GRCode::Function);
		writeU8(0); /* TODO: static and public */
		writeName(name);
		writeU16L((std::uint16_t)variable.overloadID);

		writeU16L((std::uint16_t)type.function.parameters.size());
		writeU16L((std::uint16_t)type.function.genericParameters.size());

		for (
			std::pair<Name, VariableInfo>& parameter:
			type.function.parameters
		) {
			writeU8((std::uint8_t)GRCode::FunctionParameter);
			writeName(parameter.first);
			writeTypeID(*parameter.second.type);
		}
		for (const GenericInfo& generic: type.function.genericParameters)
			writeGeneric(generic, GRCode::GenericParameter);

		writeTypeID(*type.function.returnType);
	}

	/* Write GRCode::ExternalType */
	void GRCodeSerializer::writeExternal(
		GRCode type,
		const Name& name
	) {
		writeU8((std::uint8_t)type);
		writeName(name);
	}

	/* Write GRCode::TypeID */
	void GRCodeSerializer::writeTypeID(
		const TypeID& typeID
	) {
		writeU8((std::uint8_t)GRCode::TypeID);
		writeName(typeID.name);

		writeU16L((std::uint16_t)typeID.generics.size());

		for (const GenericInfo& generic: typeID.generics)
			writeGeneric(generic, GRCode::GenericArgument);
	}

	/* Write GRCode::GenericArgument */
	void GRCodeSerializer::writeGeneric(
		const GenericInfo& generic,
		GRCode type
	) {
		writeU8((std::uint8_t)type);
		writeU8((std::uint8_t)generic.type);

		switch (generic.type) {
			case GenericType::Type:
				if (generic.substitutedType) {

					writeU8(1);
					writeTypeID(*generic.substitutedType);
				}
				else writeU8(0);
				break;
			case GenericType::Int:
				writeU8(1);
				writeU64L((std::uint64_t)generic.substitutedValue);
				break;
		}
	}

	/* Write class member */
	void GRCodeSerializer::writeClassMember(
		const Name& name,
		const VariableInfo& member
	) {
		/* TODO: fix for function pointers */
		if (member.type->type &&
		    member.type->type->base == BaseType::Function)
			writeFunction(name, member);
		else
			writeClassField(name, member);
	}
}
