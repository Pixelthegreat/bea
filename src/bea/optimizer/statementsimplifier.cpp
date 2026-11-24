/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/optimizer/statementsimplifier.hpp>

namespace bea {

	/* Visit carry result ("?" operator) */
	Result<None> StatementSimplifier::visitCarryResult(CarryResultNode& node) {

		Node& rNode = *node.m_node;
		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(rNode))
		);
		auto blockNode = std::make_unique<BlockNode>();
		blockNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		blockNode->m_atom.flagExpression = true;

		/* let result = expr */
		std::vector<Name> names = {"result"};
		auto letNode = std::make_unique<LetStatementNode>(
			std::move(names),
			nullptr,
			std::move(node.m_node)
		);
		letNode->m_atom.type = std::make_unique<TypeID>(
			*m_returnType->generics[0].substitutedType
		);
		blockNode->m_nodes.emplace_back(std::move(letNode));

		/* not result.okay */
		VariableInfo *variable =
			&m_returnType->type->object.
			publicMembers.data()[2].second;

		auto varNode1 = std::make_unique<VariableAccessNode>("result");
		varNode1->m_atom.scopeID = 1;

		auto memberNode1 = std::make_unique<NamedAccessNode>(
			std::move(varNode1), "okay"
		);
		memberNode1->m_atom.type = std::make_unique<TypeID>(
			m_context.m_booleanType
		);
		memberNode1->m_atom.variable = variable;

		static std::string s_notOperation = "not";
		auto notNode = std::make_unique<UnaryOperationNode>(
			s_notOperation, std::move(memberNode1)
		);
		notNode->m_atom.type = std::make_unique<TypeID>(
			m_context.m_booleanType
		);

		/* new Result<T>(result.error) */
		Type *errorType = m_context.getType(
			{"std", "result", "Error"}
		).value;
		variable =
			&m_returnType->type->object.
			publicMembers.data()[1].second;

		auto varNode2 = std::make_unique<VariableAccessNode>("result");
		varNode2->m_atom.scopeID = 1;

		auto memberNode2 = std::make_unique<NamedAccessNode>(
			std::move(varNode2), "error"
		);
		memberNode2->m_atom.type = std::make_unique<TypeID>(
			errorType
		);
		memberNode2->m_atom.variable = variable;

		variable =
			&m_returnType->type->object.
			publicMembers.data()[5].second;

		auto newTypeNode = std::make_unique<TypeNode>("Result");
		newTypeNode->m_atom.type = std::make_unique<TypeID>(
			*m_returnType
		);
		auto newNode = std::make_unique<NewNode>(
			std::move(newTypeNode)
		);
		newNode->m_argumentNodes.emplace_back(
			std::move(memberNode2)
		);
		newNode->m_atom.type = std::make_unique<TypeID>(
			*m_returnType
		);
		newNode->m_atom.variable = variable;

		/* if condition { return new } */
		auto blockNode2 = std::make_unique<BlockNode>();

		auto returnNode = std::make_unique<ReturnStatementNode>(
			std::move(newNode)
		);
		returnNode->m_atom.type = std::make_unique<TypeID>(
			*m_returnType
		);
		blockNode2->m_nodes.emplace_back(std::move(returnNode));

		auto ifNode = std::make_unique<IfStatementNode>(nullptr);
		Pair<std::unique_ptr<Node>> pair = {
			std::move(notNode),
			std::move(blockNode2)
		};
		ifNode->m_conditionalClauseNodes.emplace_back(
			std::move(pair)
		);
		blockNode->m_nodes.emplace_back(std::move(ifNode));

		/* result.value */
		variable =
			&m_returnType->type->object.
			publicMembers.data()[0].second;

		auto varNode3 = std::make_unique<VariableAccessNode>("result");
		varNode3->m_atom.scopeID = 1;

		auto memberNode3 = std::make_unique<NamedAccessNode>(
			std::move(varNode3), "value"
		);
		memberNode3->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		memberNode3->m_atom.variable = variable;

		blockNode->m_nodes.emplace_back(
			std::move(memberNode3)
		);
		m_optimizedNodes.emplace_back(std::move(blockNode));
		return Mode::Replace;
	}

	/* Visit "for" statement */
	Result<None> StatementSimplifier::visitForStatement(ForStatementNode& node) {

		Node& rIterableNode = *node.m_iterableNode;
		Node& rBodyNode = *node.m_bodyNode;

		node.m_iterableNode = getReplacement(
			std::move(node.m_iterableNode),
			BEA_OK(None, visit(rIterableNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(rBodyNode))
		);
		auto blockNode = std::make_unique<BlockNode>();

		/* cast<Iterable<T>>(iterable) */
		auto castTypeNode = std::make_unique<TypeNode>("Iterable");
		castTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		auto castNode = std::make_unique<CastNode>(
			std::move(castTypeNode),
			std::move(node.m_iterableNode)
		);
		castNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		castNode->m_atom.flagDynamic = true;

		/* cast.iterator() */
		Type *iterableType = m_context.getType(
			{"std", "iterator", "Iterable"}
		).value;
		Type *iteratorType = m_context.getType(
			{"std", "iterator", "Iterator"}
		).value;

		VariableInfo *variable =
			&iterableType->object.
			publicMembers.data()[0].second;

		auto memberNode = std::make_unique<NamedAccessNode>(
			std::move(castNode), "iterator"
		);
		auto callNode = std::make_unique<CallNode>(
			std::move(memberNode)
		);
		callNode->m_atom.type = std::make_unique<TypeID>(
			iteratorType
		);
		GenericInfo callGeneric{};
		callGeneric.type = GenericType::Type;
		callGeneric.substitutedType = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		callNode->m_atom.type->generics.emplace_back(
			std::move(callGeneric)
		);
		callNode->m_atom.variable = variable;

		Node& pCallNode = *callNode;

		/* let __iter = iterator */
		std::vector<Name> names = {"__iter"};
		auto letIterNode = std::make_unique<LetStatementNode>(
			std::move(names),
			nullptr, std::move(callNode)
		);
		letIterNode->m_atom.type = std::make_unique<TypeID>(
			*pCallNode.m_atom.type
		);
		letIterNode->m_atom.typeEquality = TypeEquality::Equivalent;

		blockNode->m_nodes.emplace_back(std::move(letIterNode));

		/* not __iter.isAtEnd() */
		variable =
			&iteratorType->object.
			publicMembers.data()[0].second;

		auto varNode2 = std::make_unique<VariableAccessNode>("__iter");
		varNode2->m_atom.type = std::make_unique<TypeID>(
			*pCallNode.m_atom.type
		);
		varNode2->m_atom.scopeID = node.m_atom.scopeID;

		auto memberNode2 = std::make_unique<NamedAccessNode>(
			std::move(varNode2), "isAtEnd"
		);
		auto callNode2 = std::make_unique<CallNode>(
			std::move(memberNode2)
		);
		callNode2->m_atom.type = std::make_unique<TypeID>(
			m_context.m_booleanType
		);
		callNode2->m_atom.variable = variable;

		static std::string s_notOperation = "not";
		auto notNode = std::make_unique<UnaryOperationNode>(
			s_notOperation, std::move(callNode2)
		);
		notNode->m_atom.type = std::make_unique<TypeID>(
			m_context.m_booleanType
		);

		/* __iter.next() */
		variable =
			&iteratorType->object.
			publicMembers.data()[1].second;

		auto varNode3 = std::make_unique<VariableAccessNode>("__iter");
		varNode3->m_atom.type = std::make_unique<TypeID>(
			*pCallNode.m_atom.type
		);
		varNode3->m_atom.scopeID = node.m_atom.scopeID;

		auto memberNode3 = std::make_unique<NamedAccessNode>(
			std::move(varNode3), "next"
		);
		auto callNode3 = std::make_unique<CallNode>(
			std::move(memberNode3)
		);
		callNode3->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		callNode3->m_atom.variable = variable;

		/* let item = item */
		names = {node.m_name};
		auto letNode2 = std::make_unique<LetStatementNode>(
			std::move(names),
			nullptr,
			std::move(callNode3)
		);
		letNode2->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		letNode2->m_atom.typeEquality = TypeEquality::Equivalent;

		/* while condition { body } */
		BlockNode *bodyNode = static_cast<BlockNode *>(
			node.m_bodyNode.get()
		);
		bodyNode->m_nodes.emplace(
			bodyNode->m_nodes.begin(),
			std::move(letNode2)
		);
		auto whileNode = std::make_unique<WhileStatementNode>(
			std::move(notNode),
			std::move(node.m_bodyNode)
		);
		blockNode->m_nodes.emplace_back(
			std::move(whileNode)
		);
		m_optimizedNodes.emplace_back(std::move(blockNode));
		return Mode::Replace;
	}

	/* Visit "func" statement with expression body */
	Result<None> StatementSimplifier::visitFuncExprStatement(FuncExprStatementNode& node) {

		m_returnType = node.m_returnTypeNode->m_atom.type.get();

		Node& rBodyNode = *node.m_bodyNode;
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(rBodyNode))
		);
		return Mode::Nothing;
	}

	/* Visit "func" statement with block body */
	Result<None> StatementSimplifier::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		m_returnType = node.m_returnTypeNode->m_atom.type.get();

		Node& rBodyNode = *node.m_bodyNode;
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(rBodyNode))
		);
		return Mode::Nothing;
	}
}
