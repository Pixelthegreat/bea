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
			else stream << ", ";

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
	TypeEquality Type::equalTo(
		const Type& other,
		const TypeID *typeID,
		const TypeID *otherTypeID
	) const {

		/* Object types */
		if (base == BaseType::Object) {

			if (this == &other)
				return TypeEquality::Equivalent;

			/* Check for type coercion */
			for (
				const std::pair<Name, VariableInfo>& member:
				object.publicMembers
			) {
				Type *fType = member.second.type->type;

				if (member.first != "init" || !fType ||
				    fType->base != BaseType::Function ||
				    fType->function.parameters.size() != 1)
					continue;

				TypeID& gTypeID =
					*fType->function.parameters.data()[0].
					second.type;

				std::unique_ptr<TypeID> compareTypeID =
					substituteTypes(
						gTypeID,
						this->object.genericParameters,
						typeID->generics
					);
				if (compareTypeID->equalTo(*otherTypeID) ==
				    TypeEquality::Equivalent)
					return TypeEquality::Coercible;
			}

			/* Check for implementation */
			if (other.base != BaseType::Object || !typeID)
				return TypeEquality::None;

			for (
				const ObjectImplementation& implementation:
				other.object.implementations
			) {
				if (implementation.type->equalTo(
					*typeID
				    ) != TypeEquality::Equivalent)
					continue;
				return TypeEquality::Coercible;
			}
			return TypeEquality::None;
		}

		/* Basic types */
		else if (base != other.base) {

			if ((distinct || other.distinct) &&
			    this != &other)
				return TypeEquality::None;

			else if (base > BaseType::F64 ||
				 other.base > BaseType::F64)
				return TypeEquality::None;

			return TypeEquality::Coercible;
		}

		/* Function types */
		else if (base == BaseType::Function)
			return this == &other?
				TypeEquality::Equivalent:
				TypeEquality::None;

		return TypeEquality::Equivalent;
	}

	/* Compare type identifiers */
	TypeEquality TypeID::equalTo(const TypeID& other) const {

		TypeEquality equality;
		if (!type || !other.type)
			equality =
				name == other.name?
				TypeEquality::Equivalent:
				TypeEquality::None;
		else
			equality = type->equalTo(*other.type, this, &other);

		/* Compare generics */
		if (equality < TypeEquality::Equivalent)
			return equality;

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
		return lowestGeneric;
	}

	/* Substitute generic types for type identifier */
	std::unique_ptr<TypeID> substituteTypes(
		const TypeID& typeID,
		const std::vector<GenericInfo>& genericParameters,
		const std::vector<GenericInfo>& genericArguments
	) {
		std::unique_ptr<TypeID> newTypeID =
			std::make_unique<TypeID>(typeID);

		for (std::size_t i = 0; i < genericParameters.size(); i++) {

			const GenericInfo& parameter = genericParameters[i];
			const GenericInfo& argument = genericArguments[i];

			if (parameter.type != GenericType::Type)
				continue;

			if (typeID.name.components.size() == 1 &&
			    typeID.name.components[0] == parameter.name) {

				newTypeID = std::make_unique<TypeID>(
					*argument.substitutedType
				);
				break;
			}
		}

		/* Substitute own generic arguments */
		for (std::size_t i = 0; i < typeID.generics.size(); i++) {

			if (typeID.generics[i].type != GenericType::Type)
				continue;

			std::unique_ptr<TypeID> genericTypeID =
				substituteTypes(
					*typeID.generics[i].substitutedType,
					genericParameters,
					genericArguments
				);
			newTypeID->generics[i].substitutedType =
				std::move(genericTypeID);
		}
		return std::move(newTypeID);
	}
}
