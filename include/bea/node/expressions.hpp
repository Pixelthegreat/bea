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
		VariableAccessNode(const Name& name) : m_name(name) {}
		VariableAccessNode(Name&& name) : m_name(name) {}

		const Name& getName() const { return m_name; }
	private:
		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
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
			  m_postfix(postfix) {}

		const std::string& m_operation;
		bool m_postfix;

		Node& getNode() const { return *m_node; }
	private:
		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
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
			  m_rightNode(std::move(rightNode)) {}

		const std::string& m_operation;

		Node& getLeftNode() const { return *m_leftNode; }
		Node& getRightNode() const { return *m_rightNode; }
	private:
		std::unique_ptr<Node> m_leftNode;
		std::unique_ptr<Node> m_rightNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * Carry result (doStuff()?)
	 */
	class CarryResultNode : public Node {
	public:
		CarryResultNode(std::unique_ptr<Node>&& node)
			: m_node(std::move(node)) {}

		Node& getNode() const { return *m_node; }
	private:
		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "new" expression
	 */
	class NewNode : public Node {
	public:
		NewNode(std::unique_ptr<Node>&& typeNode)
			: m_typeNode(std::move(typeNode)) {}

		std::vector<std::unique_ptr<Node>> m_argumentNodes;

		Node& getTypeNode() const { return *m_typeNode; }
	private:
		std::unique_ptr<Node> m_typeNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
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
			  m_valueNode(std::move(valueNode)) {}

		Node& getTypeNode() const { return *m_typeNode; }
		Node& getValueNode() const { return *m_valueNode; }
	private:
		std::unique_ptr<Node> m_typeNode;
		std::unique_ptr<Node> m_valueNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
