/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/printer.hpp>
#include <bea/node.hpp>

namespace bea {

	/* Make copy of atom */
	Atom Atom::copy() {

		Atom atom{};

		atom.type = type? std::make_unique<TypeID>(*type): nullptr;
		atom.typeEquality = typeEquality;
		atom.argumentTypeEqualities = argumentTypeEqualities;
		atom.operationImpl = operationImpl;
		atom.operationImpl2 = operationImpl2;
		atom.overloadID = overloadID;
		atom.variable = variable;
		atom.scopeID = scopeID;
		atom.flagDynamic = flagDynamic;
		atom.flagExpanded = flagExpanded;
		atom.flagExpression = flagExpression;

		return std::move(atom);
	}

	/* Default copy implementation */
	std::unique_ptr<Node> Node::copy() {

		return nullptr;
	}

	/* Print shorthand */
	std::ostream& operator<<(std::ostream& stream, Node& node) {

		NodePrinter(stream).visit(node);
		return stream;
	}

	/* Visit block */
	Result<None> BlockNode::visit(NodeVisitor& visitor) {

		return visitor.visitBlock(*this);
	}
}
