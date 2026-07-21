#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/controlflow.hpp>

namespace bea {

	Result<None> IfStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitIfStatement(*this);
	}

	Result<None> CaseStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitCaseStatement(*this);
	}

	Result<None> CaseLetStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitCaseLetStatement(*this);
	}

	Result<None> WhichStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitWhichStatement(*this);
	}

	Result<None> ForStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitForStatement(*this);
	}

	Result<None> WhileStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitWhileStatement(*this);
	}
}
