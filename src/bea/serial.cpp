#include <bea/core.hpp>
#include <bea/serial.hpp>

namespace bea {

	/* Write uint8 */
	void Serializer::writeU8(std::uint8_t value) {

		m_stream.write((const char *)&value, 1);
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

		std::uint8_t value = 0;
		m_stream.read((char *)&value, 1);

		return value;
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

		std::string string;
		string.resize((std::size_t)readU32L());

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

	/* Write GRCode::Let */
	void GRCodeSerializer::writeLet(
		const Name& name,
		const VariableInfo& variable
	) {
		writeU8((std::uint8_t)GRCode::Let);
		writeName(name);
		writeTypeID(*variable.type);
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
			const std::pair<Name, VariableInfo>& member:
			type.object.publicMembers
		)
			writeClassMember(member.first, member.second);

		for (
			const std::pair<Name, VariableInfo>& member:
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
			const std::pair<Name, VariableInfo>& member:
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

		std::uint8_t flags = 0;
		if (variable.flagStatic)
			flags |= 1;
		if (variable.flagAbstract)
			flags |= 2;
		writeU8(flags);

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
		const Name& name,
		const Position& position
	) {
		writeU8((std::uint8_t)type);
		writeName(name);
		writeU32L((std::uint32_t)position.line);
		writeU32L((std::uint32_t)position.column);
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
		writeString(generic.name);
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

	/* Read all GRCode operations in stream */
	Result<None> GRCodeDeserializer::readAll() {

		std::uint8_t byteOp = 1;
		do {
			byteOp = readU8();
			if (!byteOp) continue;

			GRCode op = (GRCode)byteOp;
			switch (op) {
				case GRCode::Use:
					m_aliases.emplace_back(readUse());
					break;
				case GRCode::Let:
					m_values.emplace_back(readLet());
					break;
				case GRCode::Class:
					m_types.emplace_back(readClass());
					break;
				case GRCode::Function:
					m_values.emplace_back(readFunction());
					break;
				case GRCode::ExternalType:
				case GRCode::ExternalEntity:
					{
						auto tuple = readExternal();
						std::get<0>(tuple) = op;

						m_externalReferences.emplace_back(
							std::move(tuple)
						);
					}
					break;
				default:
					return Result<None>(
						std::format(
							"Unrecognized operation "
							"{:02x} ({}:{:08x})",
							byteOp, m_moduleName,
							(std::size_t)m_stream.tellg()
						)
					);
			}

		} while (byteOp);

		return Result<None>();
	}

	/* Read GRCode::Use */
	Pair<Name> GRCodeDeserializer::readUse() {

		return {
			readName(),
			readName()
		};
	}

	/* Read GRCode::Let */
	std::pair<Name, VariableInfo> GRCodeDeserializer::readLet() {

		return {
			readName(),
			VariableInfo{
				.type = readTypeID(),
			},
		};
	}

	/* Read GRCode::Class */
#define SYNC_TYPES while (m_types.size() > typeStackEnd) ({\
			auto& pair = m_types[m_types.size()-1];\
			type->object.internalTypes[std::move(pair.first)] =\
				std::move(pair.second);\
			m_types.pop_back();\
		})

	std::pair<Name, std::unique_ptr<Type>> GRCodeDeserializer::readClass() {

		m_inClass++;

		std::uint8_t flags = readU8();

		std::pair<Name, std::unique_ptr<Type>> pair = {
			readName(), std::make_unique<Type>()
		};
		Type *type = pair.second.get();
		type->name = pair.first;

		if (flags & 1) type->object.flagAbstract = true;

		/* Read generic parameters */
		std::size_t genericParameterCount = (std::size_t)readU16L();
		std::size_t publicMemberCount = (std::size_t)readU16L();
		std::size_t privateMemberCount = (std::size_t)readU16L();
		std::size_t implementationCount = (std::size_t)readU16L();

		std::size_t typeStackEnd = m_types.size();

		for (std::size_t i = 0; i < genericParameterCount; i++)
			type->object.genericParameters.emplace_back(
				readGeneric().second
			);
		SYNC_TYPES;

		/* Read members */
		for (std::size_t i = 0; i < publicMemberCount; i++) {

			auto member = readClassMember();
			type->object.publicMembers.newEntry(std::move(member.first)) =
				std::move(member.second);
		}
		SYNC_TYPES;

		for (std::size_t i = 0; i < privateMemberCount; i++) {

			auto member = readClassMember();
			type->object.privateMembers.newEntry(std::move(member.first)) =
				std::move(member.second);
		}
		SYNC_TYPES;

		/* Read implementations */
		for (std::size_t i = 0; i < implementationCount; i++)
			type->object.implementations.emplace_back(readClassImpl());
		SYNC_TYPES;

		m_inClass--;
		return std::move(pair);
	}

	/* Read GRCode::ClassField */
	std::pair<Name, VariableInfo> GRCodeDeserializer::readClassField() {

		Name name = readName();

		VariableInfo variable = {
			.type = readTypeID(),
		};
		return {std::move(name), std::move(variable)};
	}

	/* Read GRCode::ClassImpl */
	ObjectImplementation GRCodeDeserializer::readClassImpl() {

		readU8(); /* GRCode::ClassImpl */
		std::unique_ptr<TypeID> typeID = readTypeID();

		ObjectImplementation impl = {
			.type = std::move(typeID),
		};

		std::size_t memberCount = (std::size_t)readU16L();
		for (std::size_t i = 0; i < memberCount; i++) {

			readU8(); /* GRCode::Function */
			auto pair = readFunction();

			impl.members[std::move(pair.first)] =
				std::move(pair.second);
		}
		return std::move(impl);
	}

	/* Read GRCode::Function */
	std::pair<Name, VariableInfo> GRCodeDeserializer::readFunction() {

		std::uint8_t flags = readU8();

		std::pair<Name, VariableInfo> pair;

		Name& name = pair.first;
		VariableInfo& variable = pair.second;

		variable = {};

		if (flags & 1)
			variable.flagStatic = true;
		if (flags & 2)
			variable.flagAbstract = true;

		if (m_inClass) name = readName();
		else name = readName();

		variable.overloadID = (std::size_t)readU16L();

		/* Create type */
		Name typeName = name;

		typeName.components[typeName.components.size()-1] = std::format(
			"::{}::{}",
			name.components[name.components.size()-1],
			variable.overloadID
		);
		std::unique_ptr<Type> uType = std::make_unique<Type>(
			BaseType::Function, std::move(typeName)
		);
		Type *type = uType.get();
		m_types.emplace_back(type->name, std::move(uType));

		variable.type = std::make_unique<TypeID>(type->name);
		variable.type->type = type;

		/* Read parameters */
		std::size_t parameterCount = (std::size_t)readU16L();
		std::size_t genericParameterCount = (std::size_t)readU16L();

		for (std::size_t i = 0; i < parameterCount; i++) {

			readU8(); /* GRCode::FunctionParameter */

			Name name = readName();
			std::unique_ptr<TypeID> typeID = readTypeID();

			VariableInfo& parameter =
				type->function.parameters.newEntry(std::move(name));
			parameter.type = std::move(typeID);
		}

		/* Read generic parameters */
		for (std::size_t i = 0; i < genericParameterCount; i++)
			type->function.genericParameters.emplace_back(
				readGeneric().second
			);

		/* Read return type */
		type->function.returnType = readTypeID();

		return std::move(pair);
	}

	/* Read GRCode::ExternalType or GRCode::ExternalEntity */
	std::tuple<GRCode, Name, Position> GRCodeDeserializer::readExternal() {

		return {
			GRCode::ExternalType,
			readName(),
			{
				.path = m_sourcePath.c_str(),
				.line = (long)readU32L(),
				.column = (long)readU32L(),
			},
		};
	}

	/* Read GRCode::TypeID */
	std::unique_ptr<TypeID> GRCodeDeserializer::readTypeID() {

		readU8(); /* GRCode::TypeID */

		std::unique_ptr<TypeID> typeID =
			std::make_unique<TypeID>(readName());

		std::size_t genericArgumentCount = (std::size_t)readU16L();
		for (std::size_t i = 0; i < genericArgumentCount; i++)
			typeID->generics.emplace_back(
				readGeneric().second
			);

		m_typeIDs.push_back(typeID.get());
		return std::move(typeID);
	}

	/* Read GRCode::GenericParameter or GRCode::GenericArgument */
	std::pair<GRCode, GenericInfo> GRCodeDeserializer::readGeneric() {

		GRCode op = (GRCode)readU8();

		GenericInfo info{};
		info.name = readString();
		info.type = (GenericType)readU8();

		std::uint8_t flags = readU8();
		if (flags & 1) { /* hasDefault */

			if (info.type == GenericType::Type)
				info.substitutedType = readTypeID();
			else
				info.substitutedValue = (std::intmax_t)readU64L();
		}
		return {op, std::move(info)};
	}

	/* Read GRCode::ClassField or GRCode::Function */
	std::pair<Name, VariableInfo> GRCodeDeserializer::readClassMember() {

		GRCode op = (GRCode)readU8();

		if (op == GRCode::ClassField)
			return readClassField();
		else if (op == GRCode::Function)
			return readFunction();
		return {};
	}
}
