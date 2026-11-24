/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	/*
	 * Global resolver bytecode
	 */
	enum class GRCode {
		/*
		 * Record of "use" statement
		 *
		 * uint8_t op = GRCode::Use;
		 * Name name;
		 * Name alias;
		 */
		Use = 1,

		/*
		 * Record of "let" statement
		 *
		 * uint8_t op = GRCode::Let;
		 * Name name;
		 * TypeID type;
		 */
		Let = 2,

		/*
		 * Record of "class" statement
		 *
		 * uint8_t op = GRCode::Class;
		 * uint8_t flags; 1 = abstract
		 * Name name;
		 * uint16_t genericParameterCount;
		 * uint16_t publicMemberCount;
		 * uint16_t privateMemberCount;
		 * uint16_t implementationCount;
		 * GenericParameter genericParameters[];
		 * (ClassField | Function) publicMembers[];
		 * (ClassField | Function) privateMembers[];
		 * ClassImpl implementations[];
		 */
		Class = 11,

		/*
		 * Record of class field
		 *
		 * uint8_t op = GRCode::ClassField;
		 * Name name;
		 * TypeID type;
		 */
		ClassField = 12,

		/*
		 * Record of "impl" statement
		 *
		 * uint8_t op = GRCode::ClassImpl;
		 * TypeID typeID;
		 * uint16_t memberCount;
		 * Function members[];
		 */
		ClassImpl = 13,

		/*
		 * Record of "func" statement
		 *
		 * uint8_t op = GRCode::Function;
		 * uint8_t flags; 1 = static, 2 = abstract, 4 = useDefault
		 * Name name;
		 * uint16_t overloadID;
		 * uint16_t parameterCount;
		 * uint16_t genericParameterCount;
		 * FunctionParameter parameters[];
		 * GenericParameter genericParameters[];
		 * TypeID returnType;
		 */
		Function = 21,

		/*
		 * Record of function parameter
		 *
		 * uint8_t op = GRCode::FunctionParameter;
		 * Name name;
		 * TypeID type;
		 */
		FunctionParameter = 22,

		/*
		 * Record of external reference to type
		 *
		 * uint8_t op = GRCode::ExternalType;
		 * Name name;
		 * uint32_t line;
		 * uint32_t column;
		 */
		ExternalType = 31,

		/*
		 * Record of external reference to
		 * entity / object
		 *
		 * uint8_t op = GRCode::ExternalEntity;
		 * Name name;
		 * uint32_t line;
		 * uint32_t column;
		 */
		ExternalEntity = 32,

		/*
		 * Record of type identifier
		 *
		 * uint8_t op = GRCode::TypeID;
		 * Name name;
		 * uint16_t genericArgumentCount;
		 * GenericArgument genericArguments[];
		 */
		TypeID = 41,

		/*
		 * Record of generic parameter
		 *
		 * uint8_t op = GRCode::GenericParameter;
		 * string name;
		 * uint8_t type; GenericType
		 * uint8_t flags; 1 = hasDefault
		 * uint64_t defaultValue? type == GenericType::Int
		 * TypeID defaultValue? type == GenericType::Type
		 */
		GenericParameter = 42,

		/*
		 * Record of generic argument
		 *
		 * uint8_t op = GRCode::GenericArgument;
		 * string name;
		 * uint8_t type; GenericType
		 * uint8_t flags; 1 = hasValue
		 * uint64_t value? type == GenericType::Int
		 * TypeID value? type == GenericType::Type
		 */
		GenericArgument = 43,
	};
}
