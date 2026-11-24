/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/node/visitor.hpp>

namespace bea {

	/*
	 * Node printer
	 */
	class NodePrinter : public NodeVisitor {
	public:
		NodePrinter(std::ostream& stream)
			: m_stream(stream) {}

		virtual Result<None> visitBlock(BlockNode& node) override;
		virtual Result<None> visitInteger(IntegerNode& node) override;
		virtual Result<None> visitFloat(FloatNode& node) override;
		virtual Result<None> visitString(StringNode& node) override;
		virtual Result<None> visitType(TypeNode& node) override;
		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;
		virtual Result<None> visitUnaryOperation(UnaryOperationNode& node) override;
		virtual Result<None> visitAssignment(AssignmentNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
		virtual Result<None> visitCarryResult(CarryResultNode& node) override;
		virtual Result<None> visitNew(NewNode& node) override;
		virtual Result<None> visitCast(CastNode& node) override;
		virtual Result<None> visitList(ListNode& node) override;
		virtual Result<None> visitCall(CallNode& node) override;
		virtual Result<None> visitNamedAccess(NamedAccessNode& node) override;
		virtual Result<None> visitIndexedAccess(IndexedAccessNode& node) override;
		virtual Result<None> visitRange(RangeNode& node) override;
		virtual Result<None> visitIfStatement(IfStatementNode& node) override;
		virtual Result<None> visitCaseStatement(CaseStatementNode& node) override;
		virtual Result<None> visitCaseLetStatement(CaseLetStatementNode& node) override;
		virtual Result<None> visitWhichStatement(WhichStatementNode& node) override;
		virtual Result<None> visitForStatement(ForStatementNode& node) override;
		virtual Result<None> visitWhileStatement(WhileStatementNode& node) override;
		virtual Result<None> visitReturnStatement(ReturnStatementNode& node) override;
		virtual Result<None> visitContinueStatement(ContinueStatementNode& node) override;
		virtual Result<None> visitBreakStatement(BreakStatementNode& node) override;
		virtual Result<None> visitModuleStatement(ModuleStatementNode& node) override;
		virtual Result<None> visitPubStatement(PubStatementNode& node) override;
		virtual Result<None> visitUseStatement(UseStatementNode& node) override;
		virtual Result<None> visitLetStatement(LetStatementNode& node) override;
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
		virtual Result<None> visitClassFieldStatement(ClassFieldStatementNode& node) override;
		virtual Result<None> visitImplStatement(ImplStatementNode& node) override;
		virtual Result<None> visitClassStatement(ClassStatementNode& node) override;
	private:
		std::ostream& m_stream;
		std::size_t m_tabLevel = 0;

		void visitFuncStatementCommon(FuncStatementNode& node);
	};
}
