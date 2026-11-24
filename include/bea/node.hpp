#pragma once

#include <iostream>
#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	/*
	 * Node visitor
	 */
	class NodeVisitor;

	/*
	 * Atom (analysis context)
	 */
	struct Atom {
		std::unique_ptr<TypeID> type = nullptr;
		TypeEquality typeEquality = TypeEquality::None;
		std::vector<TypeEquality> argumentTypeEqualities{};

		ObjectImplementation *operationImpl = nullptr;
		ObjectImplementation *operationImpl2 = nullptr;

		std::size_t overloadID = 0;
		VariableInfo *variable = nullptr;
		std::size_t scopeID = 0;

		bool flagDynamic = false;
		bool flagExpanded = false;
		bool flagExpression = false;
		bool flagSubstituted = false;

		Atom(bool flagExpression = false)
			: flagExpression(flagExpression) {}

		/*
		 * Make copy of atom
		 */
		Atom copy();
	};

	/*
	 * Node
	 */
	class Node {
	public:
		Node(bool flagExpression = false)
			: m_atom(flagExpression) {}

		Position m_position;
		Atom m_atom;

		virtual Result<None> visit(NodeVisitor& visitor) = 0;
		virtual std::unique_ptr<Node> copy();
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
#include <bea/node/miscellaneous.hpp>
#include <bea/node/functions.hpp>
#include <bea/node/classes.hpp>
