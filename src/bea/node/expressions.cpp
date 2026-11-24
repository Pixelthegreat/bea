/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/expressions.hpp>

namespace bea {

	Result<None> VariableAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitVariableAccess(*this);
	}

	std::unique_ptr<Node> VariableAccessNode::copy() {

		auto node = std::make_unique<VariableAccessNode>(m_name);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> UnaryOperationNode::visit(NodeVisitor& visitor) {

		return visitor.visitUnaryOperation(*this);
	}

	std::unique_ptr<Node> UnaryOperationNode::copy() {

		auto node = std::make_unique<UnaryOperationNode>(
			m_operation,
			m_node->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> AssignmentNode::visit(NodeVisitor& visitor) {

		return visitor.visitAssignment(*this);
	}

	std::unique_ptr<Node> AssignmentNode::copy() {

		auto node = std::make_unique<AssignmentNode>(
			m_operation,
			m_leftNode->copy(),
			m_rightNode->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> BinaryOperationNode::visit(NodeVisitor& visitor) {

		return visitor.visitBinaryOperation(*this);
	}

	std::unique_ptr<Node> BinaryOperationNode::copy() {

		auto node = std::make_unique<BinaryOperationNode>(
			m_operation,
			m_leftNode->copy(),
			m_rightNode->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> CarryResultNode::visit(NodeVisitor& visitor) {

		return visitor.visitCarryResult(*this);
	}

	std::unique_ptr<Node> CarryResultNode::copy() {

		auto node = std::make_unique<CarryResultNode>(
			m_node->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> NewNode::visit(NodeVisitor& visitor) {

		return visitor.visitNew(*this);
	}

	std::unique_ptr<Node> NewNode::copy() {

		auto node = std::make_unique<NewNode>(
			m_typeNode->copy()
		);
		node->m_atom = m_atom.copy();

		for (std::unique_ptr<Node>& argument: m_argumentNodes)
			node->m_argumentNodes.emplace_back(argument->copy());

		return node;
	}

	Result<None> CastNode::visit(NodeVisitor& visitor) {

		return visitor.visitCast(*this);
	}

	std::unique_ptr<Node> CastNode::copy() {

		auto node = std::make_unique<CastNode>(
			m_typeNode->copy(),
			m_valueNode->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> ListNode::visit(NodeVisitor& visitor) {

		return visitor.visitList(*this);
	}

	std::unique_ptr<Node> ListNode::copy() {

		auto node = std::make_unique<ListNode>();
		node->m_atom = m_atom.copy();

		for (std::unique_ptr<Node>& item: m_itemNodes)
			node->m_itemNodes.emplace_back(item->copy());

		return node;
	}

	Result<None> CallNode::visit(NodeVisitor& visitor) {

		return visitor.visitCall(*this);
	}

	std::unique_ptr<Node> CallNode::copy() {

		auto node = std::make_unique<CallNode>(
			m_node->copy()
		);
		node->m_atom = m_atom.copy();

		for (std::unique_ptr<Node>& argument: m_argumentNodes)
			node->m_argumentNodes.emplace_back(argument->copy());

		return node;
	}

	Result<None> NamedAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitNamedAccess(*this);
	}

	std::unique_ptr<Node> NamedAccessNode::copy() {

		auto node = std::make_unique<NamedAccessNode>(
			m_node->copy(),
			m_name
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> IndexedAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitIndexedAccess(*this);
	}

	std::unique_ptr<Node> IndexedAccessNode::copy() {

		auto node = std::make_unique<IndexedAccessNode>(
			m_node->copy(),
			m_indexNode->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> RangeNode::visit(NodeVisitor& visitor) {

		return visitor.visitRange(*this);
	}

	std::unique_ptr<Node> RangeNode::copy() {

		auto node = std::make_unique<RangeNode>(
			m_startNode->copy(),
			m_endNode->copy()
		);
		node->m_atom = m_atom.copy();

		return node;
	}
}
