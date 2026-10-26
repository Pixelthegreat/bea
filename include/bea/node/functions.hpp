#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Parameter info
	 */
	struct NodeFuncParameter {
		Name name;
		std::unique_ptr<Node> typeNode;
		std::unique_ptr<Node> valueNode;
	};

	struct NodeGenericParameter {
		Name name;
		GenericType type;
		std::unique_ptr<Node> defaultNode;
	};

	/*
	 * "func" statement base
	 */
	class FuncStatementNode : public ExportableNode {
	public:
		FuncStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& returnTypeNode
		)
			: m_name(name),
			  m_returnTypeNode(std::move(returnTypeNode)) {}

		FuncStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& returnTypeNode
		)
			: m_name(std::move(name)),
			  m_returnTypeNode(std::move(returnTypeNode)) {}

		std::vector<NodeFuncParameter> m_parameters;
		std::vector<NodeGenericParameter> m_genericParameters;
		bool m_static, m_useDefault;

		const Name& getName() const { return m_name; }
		Node& getReturnTypeNode() const { return *m_returnTypeNode; }

		Name m_name;
		std::unique_ptr<Node> m_returnTypeNode;
	};

	/*
	 * "func" statement, with expression for body (func => ...)
	 */
	class FuncExprStatementNode : public FuncStatementNode {
	public:
		FuncExprStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& returnTypeNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: FuncStatementNode(name, std::move(returnTypeNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		FuncExprStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& returnTypeNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: FuncStatementNode(std::move(name), std::move(returnTypeNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getBodyNode() const { return *m_bodyNode; }

		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "func" statement, with block for body
	 */
	class FuncBlockStatementNode : public FuncStatementNode {
	public:
		FuncBlockStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& returnTypeNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: FuncStatementNode(name, std::move(returnTypeNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		FuncBlockStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& returnTypeNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: FuncStatementNode(std::move(name), std::move(returnTypeNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getBodyNode() const { return *m_bodyNode; }

		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
