#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Node visitor
	 */
	class NodeVisitor {
	public:
		Result<None> visit(Node& node);

		virtual Result<None> visitBlock(BlockNode& node);

		/*
		 * Constants
		 */
		virtual Result<None> visitInteger(IntegerNode& node);
		virtual Result<None> visitFloat(FloatNode& node);
		virtual Result<None> visitString(StringNode& node);
		virtual Result<None> visitType(TypeNode& node);

		/*
		 * Expressions
		 */
		virtual Result<None> visitVariableAccess(VariableAccessNode& node);
		virtual Result<None> visitUnaryOperation(UnaryOperationNode& node);
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node);
		virtual Result<None> visitCarryResult(CarryResultNode& node);
		virtual Result<None> visitNew(NewNode& node);
		virtual Result<None> visitCast(CastNode& node);

		/*
		 * Control flow
		 */
		virtual Result<None> visitIfStatement(IfStatementNode& node);
		virtual Result<None> visitCaseStatement(CaseStatementNode& node);
		virtual Result<None> visitCaseLetStatement(CaseLetStatementNode& node);
		virtual Result<None> visitWhichStatement(WhichStatementNode& node);
		virtual Result<None> visitForStatement(ForStatementNode& node);
		virtual Result<None> visitWhileStatement(WhileStatementNode& node);
	};
}
