#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/node/visitor.hpp>

namespace bea {

	/*
	 * Optimizer visitor
	 */
	class NodeOptimizerVisitor : public NodeVisitor {
	public:
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
	protected:
		class Mode { /* I acknowledge this is a stupid way to create an enum */
		public:
			static const None Nothing;
			static const None Replace;
			static const None Add;
		};
		std::vector<std::unique_ptr<Node>> m_optimizedNodes;

		/*
		 * Add optimized nodes to list
		 */
		std::size_t addOptimized(
			std::vector<std::unique_ptr<Node>>& nodes,
			std::size_t index,
			None mode
		);

		/*
		 * Get replacement node
		 */
		std::unique_ptr<Node> getReplacement(
				std::unique_ptr<Node> node,
				None mode
		);
	};
}
