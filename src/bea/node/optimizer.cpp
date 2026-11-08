#include <bea/core.hpp>
#include <bea/node/optimizer.hpp>

namespace bea {

	const None NodeOptimizerVisitor::Mode::Nothing = 0;
	const None NodeOptimizerVisitor::Mode::Replace = 1;
	const None NodeOptimizerVisitor::Mode::Add = 2;

	/* Visit block */
	Result<None> NodeOptimizerVisitor::visitBlock(BlockNode& node) {

		for (std::size_t i = 0; i < node.m_nodes.size();)
			i = addOptimized(
				node.m_nodes, i,
				BEA_OK(None, visit(*node.m_nodes[i]))
			);
		return Mode::Nothing;
	}

	/* Visit integer */
	Result<None> NodeOptimizerVisitor::visitInteger(IntegerNode& node) {

		return Mode::Nothing;
	}

	/* Visit float */
	Result<None> NodeOptimizerVisitor::visitFloat(FloatNode& node) {

		return Mode::Nothing;
	}

	/* Visit string */
	Result<None> NodeOptimizerVisitor::visitString(StringNode& node) {

		return Mode::Nothing;
	}

	/* Visit type */
	Result<None> NodeOptimizerVisitor::visitType(TypeNode& node) {

		for (std::size_t i = 0; i < node.m_genericArgumentNodes.size();)
			i = addOptimized(
				node.m_genericArgumentNodes, i,
				BEA_OK(None, visit(*node.m_genericArgumentNodes[i]))
			);
		return Mode::Nothing;
	}

	/* Visit variable access */
	Result<None> NodeOptimizerVisitor::visitVariableAccess(VariableAccessNode& node) {

		return Mode::Nothing;
	}

	/* Visit unary operation */
	Result<None> NodeOptimizerVisitor::visitUnaryOperation(UnaryOperationNode& node) {

		Node& nodeP = *node.m_node;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		return Mode::Nothing;
	}

	/* Visit assignment */
	Result<None> NodeOptimizerVisitor::visitAssignment(AssignmentNode& node) {

		Node& leftNode = *node.m_leftNode;
		Node& rightNode = *node.m_rightNode;

		node.m_leftNode = getReplacement(
			std::move(node.m_leftNode),
			BEA_OK(None, visit(leftNode))
		);
		node.m_rightNode = getReplacement(
			std::move(node.m_rightNode),
			BEA_OK(None, visit(rightNode))
		);
		return Mode::Nothing;
	}

	/* Visit binary operation */
	Result<None> NodeOptimizerVisitor::visitBinaryOperation(BinaryOperationNode& node) {

		Node& leftNode = *node.m_leftNode;
		Node& rightNode = *node.m_rightNode;

		node.m_leftNode = getReplacement(
			std::move(node.m_leftNode),
			BEA_OK(None, visit(leftNode))
		);
		node.m_rightNode = getReplacement(
			std::move(node.m_rightNode),
			BEA_OK(None, visit(rightNode))
		);
		return Mode::Nothing;
	}

	/* Visit carry result */
	Result<None> NodeOptimizerVisitor::visitCarryResult(CarryResultNode& node) {

		Node& nodeP = *node.m_node;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		return Mode::Nothing;
	}

	/* Visit "new" expression */
	Result<None> NodeOptimizerVisitor::visitNew(NewNode& node) {

		Node& typeNode = *node.m_typeNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(typeNode))
		);
		for (std::size_t i = 0; i < node.m_argumentNodes.size();)
			i = addOptimized(
				node.m_argumentNodes, i,
				BEA_OK(None, visit(*node.m_argumentNodes[i]))
			);
		return Mode::Nothing;
	}

	/* Visit "cast" expression */
	Result<None> NodeOptimizerVisitor::visitCast(CastNode& node) {

		Node& typeNode = *node.m_typeNode;
		Node& valueNode = *node.m_valueNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(typeNode))
		);
		node.m_valueNode = getReplacement(
			std::move(node.m_valueNode),
			BEA_OK(None, visit(valueNode))
		);
		return Mode::Nothing;
	}

	/* Visit list initializer */
	Result<None> NodeOptimizerVisitor::visitList(ListNode& node) {

		for (std::size_t i = 0; i < node.m_itemNodes.size();)
			i = addOptimized(
				node.m_itemNodes, i,
				BEA_OK(None, visit(*node.m_itemNodes[i]))
			);
		return Mode::Nothing;
	}

	/* Visit function call */
	Result<None> NodeOptimizerVisitor::visitCall(CallNode& node) {

		Node& nodeP = *node.m_node;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		for (std::size_t i = 0; i < node.m_argumentNodes.size();)
			i = addOptimized(
				node.m_argumentNodes, i,
				BEA_OK(None, visit(*node.m_argumentNodes[i]))
			);
		return Mode::Nothing;
	}

	/* Visit named member access */
	Result<None> NodeOptimizerVisitor::visitNamedAccess(NamedAccessNode& node) {

		Node& nodeP = *node.m_node;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		return Mode::Nothing;
	}

	/* Visit indexed member access */
	Result<None> NodeOptimizerVisitor::visitIndexedAccess(IndexedAccessNode& node) {

		Node& nodeP = *node.m_node;
		Node& indexNode = *node.m_indexNode;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		node.m_indexNode = getReplacement(
			std::move(node.m_indexNode),
			BEA_OK(None, visit(indexNode))
		);
		return Mode::Nothing;
	}

	/* Visit range */
	Result<None> NodeOptimizerVisitor::visitRange(RangeNode& node) {

		Node& startNode = *node.m_startNode;
		Node& endNode = *node.m_endNode;

		node.m_startNode = getReplacement(
			std::move(node.m_startNode),
			BEA_OK(None, visit(startNode))
		);
		node.m_endNode = getReplacement(
			std::move(node.m_endNode),
			BEA_OK(None, visit(endNode))
		);
		return Mode::Nothing;
	}

	/* Visit "if" statement */
	Result<None> NodeOptimizerVisitor::visitIfStatement(IfStatementNode& node) {

		for (Pair<std::unique_ptr<Node>>& pair: node.m_conditionalClauseNodes) {

			Node& conditionNode = *std::get<0>(pair);
			Node& bodyNode = *std::get<1>(pair);

			pair = {
				getReplacement(
					std::move(std::get<0>(pair)),
					BEA_OK(None, visit(conditionNode))
				),
				getReplacement(
					std::move(std::get<1>(pair)),
					BEA_OK(None, visit(bodyNode))
				),
			};
		}
		Node& elseClauseNode = *node.m_elseClauseNode;

		node.m_elseClauseNode = getReplacement(
			std::move(node.m_elseClauseNode),
			BEA_OK(None, visit(elseClauseNode))
		);
		return Mode::Nothing;
	}

	/* Visit "case" statement */
	Result<None> NodeOptimizerVisitor::visitCaseStatement(CaseStatementNode& node) {

		Node& caseNode = *node.m_caseNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_caseNode = getReplacement(
			std::move(node.m_caseNode),
			BEA_OK(None, visit(caseNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "case let" statement */
	Result<None> NodeOptimizerVisitor::visitCaseLetStatement(CaseLetStatementNode& node) {

		Node& caseNode = *node.m_caseNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_caseNode = getReplacement(
			std::move(node.m_caseNode),
			BEA_OK(None, visit(caseNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "which" statement */
	Result<None> NodeOptimizerVisitor::visitWhichStatement(WhichStatementNode& node) {

		Node& valueNode = *node.m_valueNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_valueNode = getReplacement(
			std::move(node.m_valueNode),
			BEA_OK(None, visit(valueNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "for" statement */
	Result<None> NodeOptimizerVisitor::visitForStatement(ForStatementNode& node) {

		Node& iterableNode = *node.m_iterableNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_iterableNode = getReplacement(
			std::move(node.m_iterableNode),
			BEA_OK(None, visit(iterableNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "while" statement */
	Result<None> NodeOptimizerVisitor::visitWhileStatement(WhileStatementNode& node) {

		Node& conditionNode = *node.m_conditionNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_conditionNode = getReplacement(
			std::move(node.m_conditionNode),
			BEA_OK(None, visit(conditionNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "return" statement */
	Result<None> NodeOptimizerVisitor::visitReturnStatement(ReturnStatementNode& node) {

		return Mode::Nothing;
	}

	/* Visit "continue" statement */
	Result<None> NodeOptimizerVisitor::visitContinueStatement(ContinueStatementNode& node) {

		return Mode::Nothing;
	}

	/* Visit "break" statement */
	Result<None> NodeOptimizerVisitor::visitBreakStatement(BreakStatementNode& node) {

		return Mode::Nothing;
	}

	/* Visit "module" statement */
	Result<None> NodeOptimizerVisitor::visitModuleStatement(ModuleStatementNode& node) {

		return Mode::Nothing;
	}

	/* Visit "pub" statement */
	Result<None> NodeOptimizerVisitor::visitPubStatement(PubStatementNode& node) {

		Node& nodeP = *node.m_node;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(nodeP))
		);
		return Mode::Nothing;
	}

	/* Visit "use" statement */
	Result<None> NodeOptimizerVisitor::visitUseStatement(UseStatementNode& node) {

		return Mode::Nothing;
	}

	/* Visit "let" statement */
	Result<None> NodeOptimizerVisitor::visitLetStatement(LetStatementNode& node) {

		Node& typeNode = *node.m_typeNode;
		Node& valueNode = *node.m_valueNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(typeNode))
		);
		node.m_valueNode = getReplacement(
			std::move(node.m_valueNode),
			BEA_OK(None, visit(valueNode))
		);
		return Mode::Nothing;
	}

	/* Visit "func" statement with expression body */
	Result<None> NodeOptimizerVisitor::visitFuncExprStatement(FuncExprStatementNode& node) {

		Node *returnTypeNode = node.m_returnTypeNode.get();
		Node& bodyNode = *node.m_bodyNode;

		if (node.m_returnTypeNode)
			node.m_returnTypeNode = getReplacement(
				std::move(node.m_returnTypeNode),
				BEA_OK(None, visit(*returnTypeNode))
			);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "func" statement with block body */
	Result<None> NodeOptimizerVisitor::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		Node *returnTypeNode = node.m_returnTypeNode.get();
		Node& bodyNode = *node.m_bodyNode;

		if (node.m_returnTypeNode)
			node.m_returnTypeNode = getReplacement(
				std::move(node.m_returnTypeNode),
				BEA_OK(None, visit(*returnTypeNode))
			);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit class field statement */
	Result<None> NodeOptimizerVisitor::visitClassFieldStatement(ClassFieldStatementNode& node) {

		Node& typeNode = *node.m_typeNode;
		Node& valueNode = *node.m_valueNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(typeNode))
		);
		node.m_valueNode = getReplacement(
			std::move(node.m_valueNode),
			BEA_OK(None, visit(valueNode))
		);
		return Mode::Nothing;
	}

	/* Visit "impl" statement */
	Result<None> NodeOptimizerVisitor::visitImplStatement(ImplStatementNode& node) {

		Node& typeNode = *node.m_typeNode;
		Node& bodyNode = *node.m_bodyNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(typeNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "class" statement */
	Result<None> NodeOptimizerVisitor::visitClassStatement(ClassStatementNode& node) {

		Node& bodyNode = *node.m_bodyNode;

		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(bodyNode))
		);
		return Mode::Nothing;
	}

	/* Add optimized nodes to list */
	std::size_t NodeOptimizerVisitor::addOptimized(
		std::vector<std::unique_ptr<Node>>& nodes,
		std::size_t index,
		None mode
	) {
		switch (mode) {

			/* Nothing */
			case Mode::Nothing:
				index++;
				break;

			/* Replace current node and add children */
			case Mode::Replace:
				nodes.erase(nodes.begin() + index);

			/* Add children */
			case Mode::Add:
				if (mode != Mode::Replace) index++;

				while (!m_optimizedNodes.empty()) {

					std::unique_ptr<Node> node = std::move(
						m_optimizedNodes[m_optimizedNodes.size()-1]
					);
					m_optimizedNodes.pop_back();

					nodes.insert(
						nodes.begin() + index++,
						std::move(node)
					);
				}
				break;
		}
		return index;
	}

	/* Get replacement node */
	std::unique_ptr<Node> NodeOptimizerVisitor::getReplacement(
		std::unique_ptr<Node> node,
		None mode
	) {
		if (mode == Mode::Replace && !m_optimizedNodes.empty())
			node = std::move(m_optimizedNodes[0]);

		m_optimizedNodes.clear();
		return std::move(node);
	}
}
