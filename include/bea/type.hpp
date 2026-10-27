#pragma once

#include <bea/core.hpp>
#include <bea/container.hpp>

namespace bea {

	struct Type;

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
		DynFixed<std::string> components;

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
		const std::string *begin() const { return components.begin(); }
		const std::string *end() const { return components.end(); }

		/*
		 * Compare equality
		 */
		bool operator==(const Name& other);
	};

	std::ostream& operator<<(std::ostream& stream, const Name& name);

	/*
	 * Variable info
	 */
	enum class VariableFlag {
		Constant = 0x1,
		HasDefault = 0x2,
		Abstract = 0x4,
	};

	struct VariableInfo {
		VariableFlag flags;
		const Type *type;
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
		const Type *substitutedType;
		std::string name;

		GenericInfo() {}
	};

	/*
	 * Function type representation
	 */
	struct FunctionType {
		const Type *returnType;
		OrderedMap<Name, VariableInfo> parameters;
	};

	/*
	 * Object type representation
	 */
	enum class ObjectFlag {
		Abstract = 0,
	};

	struct ObjectType {
		ObjectFlag flags;
		OrderedMap<Name, VariableInfo> publicMembers;
		OrderedMap<Name, VariableInfo> privateMembers;
	};

	/*
	 * Type representation
	 */
	struct Type {
		bool external = false;
		BaseType base = BaseType::Object;
		bool distinct = false;
		Name name;
		DynFixed<GenericInfo> generics;
		FunctionType function;
		ObjectType object;

		Type() : generics(0) {}

		Type(BaseType base) : base(base), generics(0) {}

		Type(BaseType base, const char *name) : base(base), name(name), generics(0) {}

		Type(const std::string& name) : base(BaseType::Object), name(name), generics(0) {}

		Type(const char *name) : base(BaseType::Object), name(name), generics(0) {}

		Type(const Name& name) : base(BaseType::Object), name(name), generics(0) {}
	};

	std::ostream& operator<<(std::ostream& stream, const Type& type);
}
