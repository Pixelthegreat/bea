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

	std::ostream& operator<<(std::ostream& stream, const Name& name);

	/*
	 * Variable info
	 */
	struct VariableInfo {
		bool flagConstant;
		bool flagUseDefault;
		bool flagAbstract;
		bool flagPublic;
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
	enum class ObjectFlag {
		Abstract = 0,
	};

	struct ObjectImplementation {
		std::unique_ptr<TypeID> baseType;
		OrderedMap<Name, VariableInfo> members;
	};

	struct ObjectType {
		ObjectFlag flags;
		std::vector<GenericInfo> genericParameters;
		OrderedMap<Name, VariableInfo> publicMembers;
		OrderedMap<Name, VariableInfo> privateMembers;
		OrderedMap<Name, ObjectImplementation> implementations;

		ObjectType() : genericParameters(0) {}
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
		const Type *type;

		TypeID() : generics(0) {}

		TypeID(const std::string& name) : name(name), generics(0) {}

		TypeID(const char *name) : name(name), generics(0) {}

		TypeID(const Name& name) : name(name), generics(0) {}

		TypeID(Name&& name) : name(std::move(name)), generics(0) {}
	};

	std::ostream& operator<<(std::ostream& stream, const TypeID& typeID);
}
