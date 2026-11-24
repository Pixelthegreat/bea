/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/type.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Variable access
	 */
	class VariableAccessNode : public Node {
	public:
		VariableAccessNode(const Name& name)
			: m_name(name),
			  Node(true) {}

		VariableAccessNode(Name&& name)
			: m_name(name),
			  Node(true) {}

		const Name& getName() const { return m_name; }

		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Unary operation
	 */
	class UnaryOperationNode : public Node {
	public:
		UnaryOperationNode(
			const std::string& operation,
			std::unique_ptr<Node>&& node,
			bool postfix = false
		)
			: m_operation(operation),
			  m_node(std::move(node)),
			  m_postfix(postfix),
			  Node(true) {}

		const std::string& m_operation;
		bool m_postfix;

		Node& getNode() const { return *m_node; }

		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Assignment
	 */
	class AssignmentNode : public Node {
	public:
		AssignmentNode(
			const std::string& operation,
			std::unique_ptr<Node>&& leftNode,
			std::unique_ptr<Node>&& rightNode
		)
			: m_operation(operation),
			  m_leftNode(std::move(leftNode)),
			  m_rightNode(std::move(rightNode)),
			  Node(true) {}

		const std::string& m_operation;

		Node& getLeftNode() const { return *m_leftNode; }
		Node& getRightNode() const { return *m_rightNode; }

		std::unique_ptr<Node> m_leftNode;
		std::unique_ptr<Node> m_rightNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Binary operation
	 */
	class BinaryOperationNode : public Node {
	public:
		BinaryOperationNode(
			const std::string& operation,
			std::unique_ptr<Node>&& leftNode,
			std::unique_ptr<Node>&& rightNode
		)
			: m_operation(operation),
			  m_leftNode(std::move(leftNode)),
			  m_rightNode(std::move(rightNode)),
			  Node(true) {}

		const std::string& m_operation;

		Node& getLeftNode() const { return *m_leftNode; }
		Node& getRightNode() const { return *m_rightNode; }

		std::unique_ptr<Node> m_leftNode;
		std::unique_ptr<Node> m_rightNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Carry result (doStuff()?)
	 */
	class CarryResultNode : public Node {
	public:
		CarryResultNode(std::unique_ptr<Node>&& node)
			: m_node(std::move(node)),
			  Node(true) {}

		Node& getNode() const { return *m_node; }

		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * "new" expression
	 */
	class NewNode : public Node {
	public:
		NewNode(std::unique_ptr<Node>&& typeNode)
			: m_typeNode(std::move(typeNode)),
			  Node(true) {}

		std::vector<std::unique_ptr<Node>> m_argumentNodes;

		Node& getTypeNode() const { return *m_typeNode; }

		std::unique_ptr<Node> m_typeNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * "cast" expression
	 */
	class CastNode : public Node {
	public:
		CastNode(
			std::unique_ptr<Node>&& typeNode,
			std::unique_ptr<Node>&& valueNode
		)
			: m_typeNode(std::move(typeNode)),
			  m_valueNode(std::move(valueNode)),
			  Node(true) {}

		Node& getTypeNode() const { return *m_typeNode; }
		Node& getValueNode() const { return *m_valueNode; }

		std::unique_ptr<Node> m_typeNode;
		std::unique_ptr<Node> m_valueNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * List initializer
	 */
	class ListNode : public Node {
	public:
		ListNode() {}

		std::vector<std::unique_ptr<Node>> m_itemNodes;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Function call
	 */
	class CallNode : public Node {
	public:
		CallNode(std::unique_ptr<Node>&& node)
			: m_node(std::move(node)),
			  Node(true) {}

		std::vector<std::unique_ptr<Node>> m_argumentNodes;

		Node& getNode() const { return *m_node; }

		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Named member access
	 */
	class NamedAccessNode : public Node {
	public:
		NamedAccessNode(
			std::unique_ptr<Node>&& node,
			const Name& name
		)
			: m_node(std::move(node)),
			  m_name(name),
			  Node(true) {}

		NamedAccessNode(
			std::unique_ptr<Node>&& node,
			Name&& name
		)
			: m_node(std::move(node)),
			  m_name(std::move(name)),
			  Node(true) {}

		Node& getNode() const { return *m_node; }
		const Name& getName() const { return m_name; }

		std::unique_ptr<Node> m_node;
		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Indexed member access
	 */
	class IndexedAccessNode : public Node {
	public:
		IndexedAccessNode(
			std::unique_ptr<Node>&& node,
			std::unique_ptr<Node>&& indexNode
		)
			: m_node(std::move(node)),
			  m_indexNode(std::move(indexNode)),
			  Node(true) {}

		Node& getNode() const { return *m_node; }
		Node& getIndexNode() const { return *m_indexNode; }

		std::unique_ptr<Node> m_node;
		std::unique_ptr<Node> m_indexNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Range
	 */
	class RangeNode : public Node {
	public:
		RangeNode(
			std::unique_ptr<Node>&& startNode,
			std::unique_ptr<Node>&& endNode
		)
			: m_startNode(std::move(startNode)),
			  m_endNode(std::move(endNode)),
			  Node(true) {}

		Node& getStartNode() const { return *m_startNode; }
		Node& getEndNode() const { return *m_endNode; }

		std::unique_ptr<Node> m_startNode;
		std::unique_ptr<Node> m_endNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};
}
