#pragma once

#include <bea/core.hpp>
#include <bea/container.hpp>

namespace bea {

	struct Type;
	struct TypeID;

	/*
	 * Base types
	 */
	enum class BaseType {
		None = 0,

		U8,
		I8,
		U16,
		I16,
		U32,
		I32,
		U64,
		I64,
		F32,
		F64,

		Function,
		Object,
		Any,

		Count,

		Int = I64,
		Float = F64,
		Boolean = U8,
	};

	std::ostream& operator<<(std::ostream& stream, BaseType base);

	/*
	 * Name
	 */
	struct Name {
		std::vector<std::string> components;

		Name() : components(0) {}

		Name(std::size_t size)
			: components(size) {}

		Name(std::initializer_list<std::string> list)
			: components(list) {}

		Name(const char *value)
			: components{value} {}

		Name(std::string value)
			: components{value} {}

		const std::string *data() const { return components.data(); }
		const std::size_t size() const { return components.size(); }
		const bool empty() const { return components.empty(); }
		const std::string *begin() const { return components.data(); }
		const std::string *end() const { return components.data() + components.size(); }

		/*
		 * Compare equality
		 */
		bool compare(const Name& other);
	};

	bool operator==(const Name& a, const Name& b);
	bool operator==(const Name& a, const char *b);
	bool operator==(const Name& a, const std::string& b);

	std::ostream& operator<<(std::ostream& stream, const Name& name);

/*
 * Name hash
 */
}

template<>
struct std::hash<bea::Name> {

	std::size_t operator()(const bea::Name& name) const noexcept {

		char buffer[1024] = {0};
		std::size_t count = 0;

		for (std::size_t i = 0; i < name.components.size(); i++) {

			if (i && count + 2 <= sizeof(buffer)-1) {

				buffer[count++] = ':';
				buffer[count++] = ':';
			}

			const std::string& string = name.components[i];
			if (count + string.length() > sizeof(buffer)-1)
				break;

			std::memcpy(
				buffer+count,
				string.c_str(),
				string.length()
			);
			count += string.length();
		}
		buffer[count] = 0;
		std::string_view view(buffer);

		return std::hash<std::string_view>{}(view);
	}
};
namespace bea {

	/*
	 * Variable info
	 */
	struct VariableInfo {
		bool flagConstant;
		bool flagUseDefault;
		bool flagAbstract;
		bool flagPublic;
		bool flagStatic;
		std::size_t overloadID;
		std::unique_ptr<TypeID> type;
	};

	/*
	 * Generic type
	 */
	enum class GenericType {
		Type = 0,
		Int,

		Count,
	};

	std::ostream& operator<<(std::ostream& stream, GenericType type);

	/*
	 * Generic representation
	 */
	struct GenericInfo {
		GenericType type;
		std::unique_ptr<TypeID> substitutedType;
		std::intmax_t substitutedValue;
		std::string name;

		GenericInfo() {}
	};

	/*
	 * Function type representation
	 */
	struct FunctionType {
		std::unique_ptr<TypeID> returnType;
		OrderedMap<Name, VariableInfo> parameters;
		std::vector<GenericInfo> genericParameters;

		FunctionType() : genericParameters(0) {}
	};

	/*
	 * Object type representation
	 */
	struct ObjectImplementation {
		std::unique_ptr<TypeID> type;
		OrderedMap<std::string, VariableInfo> members;
		std::unordered_map<Name, std::unique_ptr<Type>> internalTypes;
	};

	struct ObjectType {
		bool flagAbstract;
		bool flagPublic;
		std::vector<GenericInfo> genericParameters;
		OrderedMap<std::string, VariableInfo> publicMembers;
		OrderedMap<std::string, VariableInfo> privateMembers;
		std::vector<ObjectImplementation> implementations;
		std::unordered_map<Name, std::unique_ptr<Type>> internalTypes;

		ObjectType() : genericParameters(0), internalTypes(0) {}
	};

	/*
	 * Type representation
	 */
	struct Type {
		bool external = false;
		BaseType base = BaseType::Object;
		bool distinct = false;
		Name name;
		FunctionType function;
		ObjectType object;

		Type() {}

		Type(BaseType base) : base(base) {}

		Type(BaseType base, const char *name) : base(base), name(name) {}

		Type(const std::string& name) : base(BaseType::Object), name(name) {}

		Type(const char *name) : base(BaseType::Object), name(name) {}

		Type(const Name& name) : base(BaseType::Object), name(name) {}
	};

	std::ostream& operator<<(std::ostream& stream, const Type& type);

	/*
	 * Type identifier / variation
	 */
	struct TypeID {
		Name name;
		std::vector<GenericInfo> generics;
		Type *type = nullptr;

		TypeID() : generics(0) {}

		TypeID(const std::string& name) : name(name), generics(0) {}

		TypeID(const char *name) : name(name), generics(0) {}

		TypeID(const Name& name) : name(name), generics(0) {}

		TypeID(Name&& name) : name(std::move(name)), generics(0) {}
	};

	std::ostream& operator<<(std::ostream& stream, const TypeID& typeID);
}
