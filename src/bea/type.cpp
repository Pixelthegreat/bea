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
		}
		return stream << "None";
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

	/* Print generic type */
	std::ostream& operator<<(std::ostream& stream, GenericType type) {

		switch (type) {
			using enum GenericType;
			case Type: return stream << "Type";
			case Int: return stream << "Int";
		}
		return stream;
	}

	/* Print type */
	std::ostream& operator<<(std::ostream& stream, const Type& type) {

		if (type.base == BaseType::Function)
			stream << "func ";
		else if (type.name.empty())
			stream << type.base;
		else
			stream << type.name;

		/* generics */
		std::size_t count = 0;
		for (const GenericInfo& generic: type.generics) {

			if (!count++) stream << "<";
			else stream << ", ";

			stream << generic.name << ": " << generic.type;
			if (generic.substitutedType)
				stream << " = " << *generic.substitutedType.get();
		}
		if (count) stream << ">";
		count = 0;

		/* function parameters */
		if (type.base == BaseType::Function) {

			stream << "(";
			for (const VariableInfo& parameter: type.function.parameters) {

				if (count++) stream << ", ";

				if (!parameter.name.empty())
					stream << parameter.name << ": ";
				stream << *parameter.type.get();
			}
			stream << ")";

			if (type.function.returnType)
				stream << " -> " << *type.function.returnType.get();
		}
		return stream;
	}
}
