#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/constants.hpp>

namespace bea {

	Result<None> IntegerNode::visit(NodeVisitor& visitor) {

		return visitor.visitInteger(*this);
	}

	Result<None> FloatNode::visit(NodeVisitor& visitor) {

		return visitor.visitFloat(*this);
	}

	Result<None> StringNode::visit(NodeVisitor& visitor) {

		return visitor.visitString(*this);
	}

	Result<None> TypeNode::visit(NodeVisitor& visitor) {

		return visitor.visitType(*this);
	}
}
