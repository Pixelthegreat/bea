#include <bea/core.hpp>
#include <bea/optimizer/postparsesimplifier.hpp>

namespace bea {

	static const std::string s_notOperation = "not";

	/*
	 * Visit carry result
	 *
	 * expr? -> {
	 *   let result = expr
	 *   if not result.okay {
	 *     return new Result(result.error)
	 *   }
	 *   result.value
	 * }
	 */
	Result<None> PostParseSimplifier::visitCarryResult(CarryResultNode& node) {

		/* let result = expr */
		DynFixed<Name> names = {"result"};

		auto letNode = std::make_unique<LetStatementNode>(
			std::move(names), nullptr, std::move(node.m_node)
		);

		/* return new Result(result.error) */
		auto variableNode1 = std::make_unique<VariableAccessNode>("result");
		auto memberNode1 = std::make_unique<NamedAccessNode>(
			std::move(variableNode1), "error"
		);

		auto newNode = std::make_unique<NewNode>(
			std::make_unique<TypeNode>("Result")
		);
		newNode->m_argumentNodes.emplace_back(std::move(memberNode1));

		auto returnNode = std::make_unique<ReturnStatementNode>(
			std::move(newNode)
		);
		auto innerBlockNode = std::make_unique<BlockNode>();
		innerBlockNode->m_nodes.emplace_back(std::move(returnNode));

		/* if not result.okay */
		auto variableNode2 = std::make_unique<VariableAccessNode>("result");
		auto memberNode2 = std::make_unique<NamedAccessNode>(
			std::move(variableNode2), "okay"
		);

		auto conditionNode = std::make_unique<UnaryOperationNode>(
			s_notOperation, std::move(memberNode2)
		);

		auto ifNode = std::make_unique<IfStatementNode>(nullptr);

		ifNode->m_conditionalClauseNodes.emplace_back(
			Pair<std::unique_ptr<Node>>{
				std::move(conditionNode),
				std::move(innerBlockNode),
			}
		);

		/* result.value */
		auto variableNode3 = std::make_unique<VariableAccessNode>("result");
		auto memberNode3 = std::make_unique<NamedAccessNode>(
			std::move(variableNode3), "value"
		);

		auto blockNode = std::make_unique<BlockNode>();

		blockNode->m_nodes.emplace_back(std::move(letNode));
		blockNode->m_nodes.emplace_back(std::move(ifNode));
		blockNode->m_nodes.emplace_back(std::move(memberNode3));

		m_optimizedNodes.emplace_back(std::move(blockNode));
		return Mode::Replace;
	}

	/*
	 * Visit range
	 *
	 * start..end -> new Range(start, end)
	 */
	Result<None> PostParseSimplifier::visitRange(RangeNode& node) {

		auto typeNode = std::make_unique<TypeNode>("Range");
		typeNode->m_position = node.m_position;

		auto newNode = std::make_unique<NewNode>(std::move(typeNode));
		newNode->m_position = node.m_position;

		newNode->m_argumentNodes.emplace_back(std::move(node.m_startNode));
		newNode->m_argumentNodes.emplace_back(std::move(node.m_endNode));

		m_optimizedNodes.emplace_back(std::move(newNode));
		return Mode::Replace;
	}
};
