#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/expressions.hpp>

namespace bea {

	Result<None> VariableAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitVariableAccess(*this);
	}

	Result<None> UnaryOperationNode::visit(NodeVisitor& visitor) {

		return visitor.visitUnaryOperation(*this);
	}

	Result<None> BinaryOperationNode::visit(NodeVisitor& visitor) {

		return visitor.visitBinaryOperation(*this);
	}

	Result<None> CarryResultNode::visit(NodeVisitor& visitor) {

		return visitor.visitCarryResult(*this);
	}

	Result<None> NewNode::visit(NodeVisitor& visitor) {

		return visitor.visitNew(*this);
	}

	Result<None> CastNode::visit(NodeVisitor& visitor) {

		return visitor.visitCast(*this);
	}
}
