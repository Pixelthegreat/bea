#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/printer.hpp>
#include <bea/node.hpp>

namespace bea {

	/* Make copy of atom */
	Atom Atom::copy() {

		return {
			.type = type? std::make_unique<TypeID>(*type): nullptr,
			.typeEquality = typeEquality,
			.argumentTypeEqualities = argumentTypeEqualities,
			.operationImpl = operationImpl,
			.operationImpl2 = operationImpl2,
			.overloadID = overloadID,
			.variable = variable,
			.scopeID = scopeID,
			.flagDynamic = flagDynamic,
			.flagExpanded = flagExpanded,
		};
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
