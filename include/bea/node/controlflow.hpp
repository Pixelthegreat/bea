/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * "if" statement
	 */
	class IfStatementNode : public Node {
	public:
		IfStatementNode(std::unique_ptr<Node>&& elseClauseNode)
			: m_elseClauseNode(std::move(elseClauseNode)) {}

		std::vector<Pair<std::unique_ptr<Node>>> m_conditionalClauseNodes;

		Node& getElseClauseNode() const { return *m_elseClauseNode; }

		std::unique_ptr<Node> m_elseClauseNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * Normal "case" statement
	 */
	class CaseStatementNode : public Node {
	public:
		CaseStatementNode(
			std::unique_ptr<Node>&& caseNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_caseNode(std::move(caseNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getCaseNode() const { return *m_caseNode; }
		Node& getBodyNode() const { return *m_bodyNode; }

		std::unique_ptr<Node> m_caseNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "case let" statement
	 */
	class CaseLetStatementNode : public Node {
	public:
		CaseLetStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& caseNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_name(name),
			  m_caseNode(std::move(caseNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		CaseLetStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& caseNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_name(std::move(name)),
			  m_caseNode(std::move(caseNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		const Name& getName() const { return m_name; }
		Node& getCaseNode() const { return *m_caseNode; }
		Node& getBodyNode() const { return *m_bodyNode; }

		Name m_name;
		std::unique_ptr<Node> m_caseNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "which" statement
	 */
	class WhichStatementNode : public Node {
	public:
		WhichStatementNode(
			std::unique_ptr<Node>&& valueNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_valueNode(std::move(valueNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getValueNode() const { return *m_valueNode; }
		Node& getBodyNode() const { return *m_bodyNode; }

		std::unique_ptr<Node> m_valueNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "for" statement
	 */
	class ForStatementNode : public Node {
	public:
		ForStatementNode(
			const Name& name,
			std::unique_ptr<Node>&& iterableNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_name(name),
			  m_iterableNode(std::move(iterableNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		ForStatementNode(
			Name&& name,
			std::unique_ptr<Node>&& iterableNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_name(std::move(name)),
			  m_iterableNode(std::move(iterableNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		const Name& getName() const { return m_name; }
		Node& getIterableNode() const { return *m_iterableNode; }
		Node& getBodyNode() const { return *m_bodyNode; }

		Name m_name;
		std::unique_ptr<Node> m_iterableNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "while" statement
	 */
	class WhileStatementNode : public Node {
	public:
		WhileStatementNode(
			std::unique_ptr<Node>&& conditionNode,
			std::unique_ptr<Node>&& bodyNode
		)
			: m_conditionNode(std::move(conditionNode)),
			  m_bodyNode(std::move(bodyNode)) {}

		Node& getConditionNode() const { return *m_conditionNode; }
		Node& getBodyNode() const { return *m_bodyNode; }

		std::unique_ptr<Node> m_conditionNode;
		std::unique_ptr<Node> m_bodyNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "return" statement
	 */
	class ReturnStatementNode : public Node {
	public:
		ReturnStatementNode() {}

		ReturnStatementNode(std::unique_ptr<Node>&& node) : m_node(std::move(node)) {}

		Node& getNode() const { return *m_node; }

		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "continue" statement
	 */
	class ContinueStatementNode : public Node {
	public:
		ContinueStatementNode() {}
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "break" statement
	 */
	class BreakStatementNode : public Node {
	public:
		BreakStatementNode() {}
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
