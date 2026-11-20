#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/constants.hpp>

namespace bea {

	Result<None> IntegerNode::visit(NodeVisitor& visitor) {

		return visitor.visitInteger(*this);
	}

	std::unique_ptr<Node> IntegerNode::copy() {

		auto node = std::make_unique<IntegerNode>(m_value);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> FloatNode::visit(NodeVisitor& visitor) {

		return visitor.visitFloat(*this);
	}

	std::unique_ptr<Node> FloatNode::copy() {

		auto node = std::make_unique<FloatNode>(m_value);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> StringNode::visit(NodeVisitor& visitor) {

		return visitor.visitString(*this);
	}

	std::unique_ptr<Node> StringNode::copy() {

		auto node = std::make_unique<StringNode>(m_value);
		node->m_atom = m_atom.copy();

		return node;
	}

	Result<None> TypeNode::visit(NodeVisitor& visitor) {

		return visitor.visitType(*this);
	}

	std::unique_ptr<Node> TypeNode::copy() {

		auto node = std::make_unique<TypeNode>(m_name);
		node->m_atom = m_atom.copy();

		for (std::unique_ptr<Node>& argument: m_genericArgumentNodes)
			node->m_genericArgumentNodes.emplace_back(argument->copy());

		return node;
	}
}
