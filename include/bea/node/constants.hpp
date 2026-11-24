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
	 * Integer constant
	 */
	class IntegerNode : public Node {
	public:
		IntegerNode(std::intmax_t value)
			: m_value(value),
			  Node(true) {}

		std::intmax_t m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Floating point constant
	 */
	class FloatNode : public Node {
	public:
		FloatNode(double value)
			: m_value(value),
			  Node(true) {}

		double m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * String constant
	 */
	class StringNode : public Node {
	public:
		StringNode(const std::string& value)
			: m_value(value),
			  Node(true) {}

		const std::string& m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};

	/*
	 * Type
	 */
	class TypeNode : public Node {
	public:
		TypeNode(const Name& name) : m_name(name) {}
		TypeNode(Name&& name) : m_name(name) {}

		std::vector<std::unique_ptr<Node>> m_genericArgumentNodes;

		const Name& getName() const { return m_name; }

		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
		virtual std::unique_ptr<Node> copy() override;
	};
}
