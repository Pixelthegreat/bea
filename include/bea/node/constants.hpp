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
		IntegerNode(std::intmax_t value) : m_value(value) {}

		std::intmax_t m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * Floating point constant
	 */
	class FloatNode : public Node {
	public:
		FloatNode(double value) : m_value(value) {}

		double m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * String constant
	 */
	class StringNode : public Node {
	public:
		StringNode(const std::string& value) : m_value(value) {}

		const std::string& m_value;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};

	/*
	 * Type
	 */
	class TypeNode : public Node {
	public:
		TypeNode(const Name& name) : m_name(name) {}
		TypeNode(Name&& name) : m_name(name) {}

		std::vector<std::unique_ptr<Node>> m_genericArguments;

		const Name& getName() const { return m_name; }
	private:
		Name m_name;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}
