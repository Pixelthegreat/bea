#pragma once

#include <iostream>
#include <bea/core.hpp>

namespace bea {

	/*
	 * Node visitor
	 */
	class NodeVisitor;

	/*
	 * Node
	 */
	class Node {
	public:
		virtual Result<None> visit(NodeVisitor& visitor) = 0;
	};

	std::ostream& operator<<(std::ostream& stream, Node& node);

	/*
	 * Block node
	 */
	class BlockNode : public Node {
	public:
		BlockNode() {}

		std::vector<std::unique_ptr<Node>> m_nodes;
	protected:
		virtual Result<None> visit(NodeVisitor& visitor) override;
	};
}

#include <bea/node/constants.hpp>
#include <bea/node/expressions.hpp>
#include <bea/node/controlflow.hpp>
