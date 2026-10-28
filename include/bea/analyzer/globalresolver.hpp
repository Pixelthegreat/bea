#pragma once

#include <bea/core.hpp>
#include <bea/serial.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

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
		 * Record of "class" statement
		 *
		 * uint8_t op = GRCode::Class;
		 * uint8_t flags; 1 = public, 2 = abstract
		 * Name name;
		 * uint16_t publicMemberCount;
		 * uint16_t privateMemberCount;
		 * uint16_t implementationCount;
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
		 * uint16_t functionCount;
		 * Function functions[];
		 */
		ClassImpl = 13,

		/*
		 * Record of "func" statement
		 *
		 * uint8_t op = GRCode::Function;
		 * Name name;
		 * uint16_t parameterCount;
		 * FunctionParameter parameters[];
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
		 */
		ExternalType = 31,

		/*
		 * Record of external reference to
		 * entity / object
		 *
		 * uint8_t op = GRCode::ExternalEntity;
		 * Name name;
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
		 * uint8_t type; GenericType
		 * uint64_t defaultValue? type == GenericType::Int
		 * TypeID defaultValue? type == GenericType::Type
		 */
		GenericParameter = 42,

		/*
		 * Record of generic argument
		 *
		 * uint8_t op = GRCode::GenericArgument;
		 * uint8_t type; GenericType
		 * uint64_t value? type == GenericType::Int
		 * TypeID value? type == GenericType::Type
		 */
		GenericArgument = 43,
	};

	/*
	 * This analyzer declares global variables,
	 * functions and types and serializes them.
	 */
	class GlobalResolver : public NodeAnalyzerVisitor {
	public:
		GlobalResolver(
			Context& context,
			Serializer& serializer
		)
			: NodeAnalyzerVisitor(context),
			  m_serializer(serializer) {}

		virtual Result<None> analyze(Node& node) override;

		virtual Result<None> visitBlock(BlockNode& node) override;
		virtual Result<None> visitUseStatement(UseStatementNode& node) override;

		Result<None> visitFuncStatement(FuncStatementNode& node);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
	private:
		Serializer& m_serializer;
	};
}
