#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/printer.hpp>
#include <bea/node.hpp>

namespace bea {

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
