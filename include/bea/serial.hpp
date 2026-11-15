#pragma once

#include <fstream>
#include <bea/type.hpp>
#include <bea/bytecode.hpp>

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
	protected:
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
	protected:
		std::istream& m_stream;
	};

	/*
	 * GRCode serializer
	 */
	class GRCodeSerializer : public Serializer {
	public:
		GRCodeSerializer(std::ostream& stream)
			: Serializer(stream) {}

		/*
		 * Write GRCode operations
		 */
		void writeUse(
			const Name& name,
			const Name& alias
		);
		void writeClass(
			const Name& name,
			const Type& type
		);
		void writeClassField(
			const Name& name,
			const VariableInfo& field
		);
		void writeClassImpl(
			const ObjectImplementation& implementation
		);
		void writeFunction(
			const Name& name,
			const VariableInfo& variable
		);
		void writeExternal(
			GRCode type,
			const Name& name,
			const Position& position
		);
		void writeTypeID(
			const TypeID& typeID
		);
		void writeGeneric(
			const GenericInfo& generic,
			GRCode type
		);
		void writeClassMember(
			const Name& name,
			const VariableInfo& member
		);
	};

	/*
	 * GRCode deserializer
	 */
	class GRCodeDeserializer : public Deserializer {
	public:
		GRCodeDeserializer(
			std::istream& stream,
			const Name& moduleName,
			const std::string& sourcePath
		)
			: Deserializer(stream),
			  m_moduleName(moduleName),
			  m_sourcePath(sourcePath) {}

		std::vector<Pair<Name>> m_aliases;
		std::vector<std::pair<Name, std::unique_ptr<Type>>> m_types;
		std::vector<std::pair<Name, VariableInfo>> m_values;
		std::vector<TypeID *> m_typeIDs;
		std::vector<std::tuple<GRCode, Name, Position>> m_externalReferences;

		/*
		 * Read GRCode operations
		 */
		Result<None> readAll();
	private:
		const Name& m_moduleName;
		const std::string& m_sourcePath;
		std::size_t m_inClass = 0;

		Pair<Name> readUse();
		std::pair<Name, std::unique_ptr<Type>> readClass();
		std::pair<Name, VariableInfo> readClassField();
		ObjectImplementation readClassImpl();
		std::pair<Name, VariableInfo> readFunction();
		std::tuple<GRCode, Name, Position> readExternal();
		std::unique_ptr<TypeID> readTypeID();
		std::pair<GRCode, GenericInfo> readGeneric();
		std::pair<Name, VariableInfo> readClassMember();
	};
};
