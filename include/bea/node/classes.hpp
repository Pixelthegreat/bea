#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Class field statement
	 */
	class ClassFieldStatementNode : public ExportableNode {
	public:
		ClassFieldStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& typeNode,
			std::unique_ptr<Node>&& valueNode
		)
			: m_name(name),
			  m_typeNode(std::move(typeNode)),
			  m_valueNode(std::move(valueNode)) {}

		ClassFieldStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& typeNode,
			std::unique_ptr<Node>&& valueNode
		)
			: m_name(std::move(name)),
			  m_typeNode(std::move(typeNode)),
			  m_valueNode(std::move(valueNode)) {}

		const Name& getName() const { return m_name; }
		Node& getTypeNode() const { return *m_typeNode; }
		Node& getValueNode() const { return *m_valueNode; }
	private:
		Name m_name;
		std::unique_ptr<Node> m_typeNode;
		std::unique_ptr<Node> m_valueNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "impl" statement
	 */
	class ImplStatementNode : public Node {
	public:
		ImplStatementNode(
			std::unique_ptr<Node>&& typeNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_typeNode(std::move(typeNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getTypeNode() const { return *m_typeNode; }
		Node& getBodyNode() const { return *m_bodyNode; }
	private:
		std::unique_ptr<Node> m_typeNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "class" statement
	 */
	class ClassStatementNode : public Node {
	public:
		ClassStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& bodyNode,
			bool copy = false
		)
			: m_name(name),
			  m_bodyNode(std::move(bodyNode)),
			  m_copy(copy) {}

		ClassStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& bodyNode,
			bool copy = false
		)
			: m_name(std::move(name)),
			  m_bodyNode(std::move(bodyNode)),
			  m_copy(copy) {}

		bool m_copy;

		const Name& getName() const { return m_name; }
		Node& getBodyNode() const { return *m_bodyNode; }
	private:
		Name m_name;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
