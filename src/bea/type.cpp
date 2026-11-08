#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	/* Print base type name */
	std::ostream& operator<<(std::ostream& stream, BaseType base) {

		switch (base) {
			using enum BaseType;
			case U8: return stream << "U8";
			case I8: return stream << "I8";
			case U16: return stream << "U16";
			case I16: return stream << "I16";
			case U32: return stream << "U32";
			case I32: return stream << "I32";
			case U64: return stream << "U64";
			case I64: return stream << "I64";
			case F32: return stream << "F32";
			case F64: return stream << "F64";
			case Function: return stream << "Function";
			case Object: return stream << "Object";
			case Any: return stream << "Any";
		}
		return stream << "None";
	}

	/* Print generic type */
	std::ostream& operator<<(std::ostream& stream, GenericType type) {

		switch (type) {
			using enum GenericType;
			case Type: return stream << "Type";
			case Int: return stream << "Int";
		}
		return stream;
	}

	/* Print name */
	std::ostream& operator<<(std::ostream& stream, const Name& name) {

		std::size_t index = 0;
		for (const std::string& component: name) {

			if (index++) stream << "::";
			stream << component;
		}
		return stream;
	}

	/* Print type */
	std::ostream& operator<<(std::ostream& stream, const Type& type) {

		const std::vector<GenericInfo> *genericParameters = nullptr;

		/* generics */
		std::size_t count = 0;
		if (type.base == BaseType::Function)
			genericParameters = &type.function.genericParameters;
		else if (type.base == BaseType::Object)
			genericParameters = &type.object.genericParameters;

		if (genericParameters) {

			for (const GenericInfo& parameter: type.object.genericParameters) {

				if (!count++) stream << "<";
				else stream << ", ";

				stream << parameter.name << ": " << parameter.type;
				if (parameter.substitutedType)
					stream << " = " << *parameter.substitutedType;
			}
			if (count) stream << ">";
		}
		count = 0;

		/* function parameters */
		if (type.base == BaseType::Function) {

			stream << "(";

			for (
				const std::pair<Name, VariableInfo>& parameter:
				type.function.parameters
			) {
				if (count++) stream << ", ";
				stream << parameter.first << ": " << *parameter.second.type;
			}

			stream << ")";

			if (type.function.returnType)
				stream << " -> " << *type.function.returnType;
		}
		return stream;
	}

	/* Print type identifier */
	std::ostream& operator<<(std::ostream& stream, const TypeID& typeID) {

		stream << typeID.name;

		std::size_t count = 0;
		for (const GenericInfo& generic: typeID.generics) {

			if (!count++) stream << "<";

			switch (generic.type) {
				case GenericType::Type:
					stream << *generic.substitutedType;
					break;
				case GenericType::Int:
					stream << generic.substitutedValue;
					break;
			}
		}
		if (count) stream << ">";

		if (typeID.type) stream << *typeID.type;

		return stream;
	}

	/* Compare equality */
	bool Name::compare(const Name& other) {

		if (components.size() != other.components.size())
			return false;

		for (std::size_t i = 0; i < components.size(); i++) {
			if (components[i] != other.components[i])
				return false;
		}
		return true;
	}

	bool operator==(const Name& a, const Name& b) {

		return std::hash<Name>{}(a) == std::hash<Name>{}(b);
	}

	bool operator==(const Name& a, const char *b) {

		std::string_view view(b);

		return std::hash<Name>{}(a) == std::hash<std::string_view>{}(view);
	}

	bool operator==(const Name& a, const std::string& b) {

		return std::hash<Name>{}(a) == std::hash<std::string>{}(b);
	}

	/* Truthiness of type equality */
	bool operator!(TypeEquality equality) {

		return equality == TypeEquality::None;
	}

	/* Compare types */
	TypeEquality Type::equalTo(const Type& other) const {

		if (base != other.base) {

			if (base > BaseType::F64 ||
			    other.base > BaseType::F64)
				return TypeEquality::None;

			if ((distinct || other.distinct) &&
			    this != &other)
				return TypeEquality::None;

			return TypeEquality::Coercible;
		}

		/* Function types */
		if (base == BaseType::Function ||
		    base == BaseType::Any)
			return this == &other?
				TypeEquality::Equivalent:
				TypeEquality::None;

		/* Object types */
		else if (base == BaseType::Object) {

			if (this == &other)
				return TypeEquality::Equivalent;

			/* Check for type coercion */
			for (
				const ObjectImplementation& implementation:
				object.implementations
			) {
				if (implementation.type->name != "std::coerce::Coerce" ||
				    implementation.type->generics.size() != 1 ||
				    !implementation.type->generics[0].substitutedType)
					continue;
				TypeID& typeID = *implementation.type;

				if (typeID.generics[0].substitutedType->type == this)
					return TypeEquality::Coercible;
			}
			return TypeEquality::None;
		}
		return TypeEquality::Equivalent;
	}

	/* Compare type identifiers */
	TypeEquality TypeID::equalTo(const TypeID& other) const {

		if (generics.size() != other.generics.size())
			return TypeEquality::None;
		TypeEquality lowestGeneric = TypeEquality::Equivalent;

		for (std::size_t i = 0; i < generics.size(); i++) {

			if (generics[i].type != other.generics[i].type)
				return TypeEquality::None;

			if (generics[i].type == GenericType::Int) {

				if (generics[i].substitutedValue !=
				    other.generics[i].substitutedValue)
					lowestGeneric = TypeEquality::None;
				continue;
			}

			/* Compare generics */
			TypeEquality equality = generics[i].substitutedType->equalTo(
				*other.generics[i].substitutedType
			);
			if (equality < lowestGeneric)
				lowestGeneric = equality;
		}

		/* Compare underlying types */
		TypeEquality equality;
		if (!type || !other.type)
			equality =
				name == other.name?
				TypeEquality::Equivalent:
				TypeEquality::None;
		else
			equality = type->equalTo(*other.type);

		return lowestGeneric < equality? lowestGeneric: equality;
	}
}
