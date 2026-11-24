/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/token.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Exportable node (Compatible with "pub" statement)
	 */
	class ExportableNode : public Node {
	public:
		bool m_public = false, m_abstract = false;
		const Token *m_commentToken = nullptr;
	};

	/*
	 * "module" statement
	 */
	class ModuleStatementNode : public Node {
	public:
		ModuleStatementNode(const Name& name) : m_name(name) {}
		ModuleStatementNode(Name&& name) : m_name(std::move(name)) {}

		const Name& getName() const { return m_name; }

		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "pub" statement
	 */
	class PubStatementNode : public Node {
	public:
		PubStatementNode(std::unique_ptr<Node>&& node)
			: m_node(std::move(node)) {}

		Node& getNode() const { return *m_node; }

		std::unique_ptr<Node> m_node;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "use" statement
	 */
	class UseStatementNode : public Node {
	public:
		UseStatementNode(const Name& name) : m_name(name) {}
		UseStatementNode(Name&& name) : m_name(std::move(name)) {}

		UseStatementNode(
			const Name& name,
			const Name& alias
		)
			: m_name(name),
			  m_alias(alias) {}

		UseStatementNode(
			Name&& name,
			Name&& alias
		)
			: m_name(std::move(name)),
			  m_alias(std::move(alias)) {}

		const Name& getName() const { return m_name; }
		const Name& getAlias() const { return m_alias; }

		Name m_name, m_alias;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * "let" statement
	 */
	class LetStatementNode : public ExportableNode {
	public:
		LetStatementNode(
			const std::vector<Name>& names,
			std::unique_ptr<Node>&& typeNode = nullptr,
			std::unique_ptr<Node>&& valueNode = nullptr
		)
			: m_names(names),
			  m_typeNode(std::move(typeNode)),
			  m_valueNode(std::move(valueNode)) {}

		LetStatementNode(
			std::vector<Name>&& names,
			std::unique_ptr<Node>&& typeNode = nullptr,
			std::unique_ptr<Node>&& valueNode = nullptr
		)
			: m_names(std::move(names)),
			  m_typeNode(std::move(typeNode)),
			  m_valueNode(std::move(valueNode)) {}

		const std::vector<Name>& getNames() const { return m_names; }
		Node& getTypeNode() const { return *m_typeNode; }
		Node& getValueNode() const { return *m_valueNode; }

		std::vector<Name> m_names;
		std::unique_ptr<Node> m_typeNode;
		std::unique_ptr<Node> m_valueNode;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
