#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Node visitor
	 */
	class NodeVisitor {
	public:
		virtual Result<None> visit(Node& node);

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
		virtual Result<None> visitAssignment(AssignmentNode& node);
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node);
		virtual Result<None> visitCarryResult(CarryResultNode& node);
		virtual Result<None> visitNew(NewNode& node);
		virtual Result<None> visitCast(CastNode& node);
		virtual Result<None> visitList(ListNode& node);
		virtual Result<None> visitCall(CallNode& node);
		virtual Result<None> visitNamedAccess(NamedAccessNode& node);
		virtual Result<None> visitIndexedAccess(IndexedAccessNode& node);
		virtual Result<None> visitRange(RangeNode& node);

		/*
		 * Control flow statements
		 */
		virtual Result<None> visitIfStatement(IfStatementNode& node);
		virtual Result<None> visitCaseStatement(CaseStatementNode& node);
		virtual Result<None> visitCaseLetStatement(CaseLetStatementNode& node);
		virtual Result<None> visitWhichStatement(WhichStatementNode& node);
		virtual Result<None> visitForStatement(ForStatementNode& node);
		virtual Result<None> visitWhileStatement(WhileStatementNode& node);
		virtual Result<None> visitReturnStatement(ReturnStatementNode& node);
		virtual Result<None> visitContinueStatement(ContinueStatementNode& node);
		virtual Result<None> visitBreakStatement(BreakStatementNode& node);

		/*
		 * Miscellaneous statements
		 */
		virtual Result<None> visitModuleStatement(ModuleStatementNode& node);
		virtual Result<None> visitPubStatement(PubStatementNode& node);
		virtual Result<None> visitUseStatement(UseStatementNode& node);
		virtual Result<None> visitLetStatement(LetStatementNode& node);

		/*
		 * Functions and classes
		 */
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node);
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node);
		virtual Result<None> visitClassFieldStatement(ClassFieldStatementNode& node);
		virtual Result<None> visitImplStatement(ImplStatementNode& node);
		virtual Result<None> visitClassStatement(ClassStatementNode& node);
	};
}
