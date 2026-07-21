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

	Result<None> AssignmentNode::visit(NodeVisitor& visitor) {

		return visitor.visitAssignment(*this);
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

	Result<None> ListNode::visit(NodeVisitor& visitor) {

		return visitor.visitList(*this);
	}

	Result<None> CallNode::visit(NodeVisitor& visitor) {

		return visitor.visitCall(*this);
	}

	Result<None> NamedAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitNamedAccess(*this);
	}

	Result<None> IndexedAccessNode::visit(NodeVisitor& visitor) {

		return visitor.visitIndexedAccess(*this);
	}

	Result<None> RangeNode::visit(NodeVisitor& visitor) {

		return visitor.visitRange(*this);
	}
}
