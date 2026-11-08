#include <bea/core.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/* Analyze node */
	Result<None> NodeAnalyzerVisitor::analyze(Node& node) {

		return visit(node);
	}

	/* Visit block */
	Result<None> NodeAnalyzerVisitor::visitBlock(BlockNode& node) {

		m_blockLevel++;

		for (std::size_t i = 0; i < node.m_nodes.size(); i++)
			BEA_OK(None, visit(*node.m_nodes[i]));

		m_blockLevel--;
		return Result<None>();
	}

	/* Visit integer */
	Result<None> NodeAnalyzerVisitor::visitInteger(IntegerNode& node) {

		return Result<None>();
	}

	/* Visit float */
	Result<None> NodeAnalyzerVisitor::visitFloat(FloatNode& node) {

		return Result<None>();
	}

	/* Visit string */
	Result<None> NodeAnalyzerVisitor::visitString(StringNode& node) {

		return Result<None>();
	}

	/* Visit type */
	Result<None> NodeAnalyzerVisitor::visitType(TypeNode& node) {

		for (std::size_t i = 0; i < node.m_genericArgumentNodes.size(); i++)
			BEA_OK(None, visit(*node.m_genericArgumentNodes[i]));
		return Result<None>();
	}

	/* Visit variable access */
	Result<None> NodeAnalyzerVisitor::visitVariableAccess(VariableAccessNode& node) {

		return Result<None>();
	}

	/* Visit unary operation */
	Result<None> NodeAnalyzerVisitor::visitUnaryOperation(UnaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_node));

		return Result<None>();
	}

	/* Visit assignment */
	Result<None> NodeAnalyzerVisitor::visitAssignment(AssignmentNode& node) {

		BEA_OK(None, visit(*node.m_leftNode));
		BEA_OK(None, visit(*node.m_rightNode));

		return Result<None>();
	}

	/* Visit binary operation */
	Result<None> NodeAnalyzerVisitor::visitBinaryOperation(BinaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_leftNode));
		BEA_OK(None, visit(*node.m_rightNode));

		return Result<None>();
	}

	/* Visit carry result */
	Result<None> NodeAnalyzerVisitor::visitCarryResult(CarryResultNode& node) {

		BEA_OK(None, visit(*node.m_node));

		return Result<None>();
	}

	/* Visit "new" expression */
	Result<None> NodeAnalyzerVisitor::visitNew(NewNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));

		for (std::size_t i = 0; i < node.m_argumentNodes.size(); i++)
			BEA_OK(None, visit(*node.m_argumentNodes[i]));

		return Result<None>();
	}

	/* Visit "cast" expression */
	Result<None> NodeAnalyzerVisitor::visitCast(CastNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		BEA_OK(None, visit(*node.m_valueNode));

		return Result<None>();
	}

	/* Visit list initializer */
	Result<None> NodeAnalyzerVisitor::visitList(ListNode& node) {

		for (std::size_t i = 0; i < node.m_itemNodes.size(); i++)
			BEA_OK(None, visit(*node.m_itemNodes[i]));
		return Result<None>();
	}

	/* Visit function call */
	Result<None> NodeAnalyzerVisitor::visitCall(CallNode& node) {

		BEA_OK(None, visit(*node.m_node));

		for (std::size_t i = 0; i < node.m_argumentNodes.size(); i++)
			BEA_OK(None, visit(*node.m_argumentNodes[i]));

		return Result<None>();
	}

	/* Visit named member access */
	Result<None> NodeAnalyzerVisitor::visitNamedAccess(NamedAccessNode& node) {

		BEA_OK(None, visit(*node.m_node));

		return Result<None>();
	}

	/* Visit indexed member access */
	Result<None> NodeAnalyzerVisitor::visitIndexedAccess(IndexedAccessNode& node) {

		BEA_OK(None, visit(*node.m_node));
		BEA_OK(None, visit(*node.m_indexNode));

		return Result<None>();
	}

	/* Visit range */
	Result<None> NodeAnalyzerVisitor::visitRange(RangeNode& node) {

		BEA_OK(None, visit(*node.m_startNode));
		BEA_OK(None, visit(*node.m_endNode));

		return Result<None>();
	}

	/* Visit "if" statement */
	Result<None> NodeAnalyzerVisitor::visitIfStatement(IfStatementNode& node) {

		for (Pair<std::unique_ptr<Node>>& pair: node.m_conditionalClauseNodes) {

			BEA_OK(None, visit(*std::get<0>(pair)));
			BEA_OK(None, visit(*std::get<1>(pair)));
		}
		BEA_OK(None, visit(*node.m_elseClauseNode));

		return Result<None>();
	}

	/* Visit "case" statement */
	Result<None> NodeAnalyzerVisitor::visitCaseStatement(CaseStatementNode& node) {

		BEA_OK(None, visit(*node.m_caseNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "case let" statement */
	Result<None> NodeAnalyzerVisitor::visitCaseLetStatement(CaseLetStatementNode& node) {

		BEA_OK(None, visit(*node.m_caseNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "which" statement */
	Result<None> NodeAnalyzerVisitor::visitWhichStatement(WhichStatementNode& node) {

		BEA_OK(None, visit(*node.m_valueNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "for" statement */
	Result<None> NodeAnalyzerVisitor::visitForStatement(ForStatementNode& node) {

		BEA_OK(None, visit(*node.m_iterableNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "while" statement */
	Result<None> NodeAnalyzerVisitor::visitWhileStatement(WhileStatementNode& node) {

		BEA_OK(None, visit(*node.m_conditionNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "return" statement */
	Result<None> NodeAnalyzerVisitor::visitReturnStatement(ReturnStatementNode& node) {

		return Result<None>();
	}

	/* Visit "continue" statement */
	Result<None> NodeAnalyzerVisitor::visitContinueStatement(ContinueStatementNode& node) {

		return Result<None>();
	}

	/* Visit "break" statement */
	Result<None> NodeAnalyzerVisitor::visitBreakStatement(BreakStatementNode& node) {

		return Result<None>();
	}

	/* Visit "module" statement */
	Result<None> NodeAnalyzerVisitor::visitModuleStatement(ModuleStatementNode& node) {

		return Result<None>();
	}

	/* Visit "pub" statement */
	Result<None> NodeAnalyzerVisitor::visitPubStatement(PubStatementNode& node) {

		BEA_OK(None, visit(*node.m_node));

		return Result<None>();
	}

	/* Visit "use" statement */
	Result<None> NodeAnalyzerVisitor::visitUseStatement(UseStatementNode& node) {

		return Result<None>();
	}

	/* Visit "let" statement */
	Result<None> NodeAnalyzerVisitor::visitLetStatement(LetStatementNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		BEA_OK(None, visit(*node.m_valueNode));

		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> NodeAnalyzerVisitor::visitFuncExprStatement(FuncExprStatementNode& node) {

		BEA_OK(None, visit(*node.m_returnTypeNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "func" statement with block body */
	Result<None> NodeAnalyzerVisitor::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		BEA_OK(None, visit(*node.m_returnTypeNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit class field statement */
	Result<None> NodeAnalyzerVisitor::visitClassFieldStatement(ClassFieldStatementNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		BEA_OK(None, visit(*node.m_valueNode));

		return Result<None>();
	}

	/* Visit "impl" statement */
	Result<None> NodeAnalyzerVisitor::visitImplStatement(ImplStatementNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> NodeAnalyzerVisitor::visitClassStatement(ClassStatementNode& node) {

		BEA_OK(None, visit(*node.m_bodyNode));

		return Result<None>();
	}
}
