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
		std::unique_ptr<Type> type;
		Name name;
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
		std::unique_ptr<Type> substitutedType;
		std::string name;

		GenericInfo() {}
	};

	/*
	 * Function type representation
	 */
	struct FunctionType {
		std::unique_ptr<Type> returnType;
		DynFixed<VariableInfo> parameters;

		FunctionType() : parameters(0) {}
	};

	/*
	 * Object type representation
	 */
	enum class ObjectFlag {
		Abstract = 0,
	};

	struct ObjectType {
		ObjectFlag flags;
		DynFixed<VariableInfo> publicMembers;
		DynFixed<VariableInfo> privateMembers;

		ObjectType() : publicMembers(0), privateMembers(0) {}
	};

	/*
	 * Type representation
	 */
	struct Type {
		BaseType base;
		bool distinct;
		Name name;
		DynFixed<GenericInfo> generics;
		FunctionType function;
		ObjectType object;

		Type() : generics(0) {}

		Type(BaseType base) : base(base), generics(0) {}

		Type(std::string name) : base(BaseType::Object), name(name), generics(0) {}

		Type(const char *name) : base(BaseType::Object), name(name), generics(0) {}

		Type(const Name& name) : base(BaseType::Object), name(name), generics(0) {}
	};

	std::ostream& operator<<(std::ostream& stream, const Type& type);
}
