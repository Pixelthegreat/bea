#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/miscellaneous.hpp>

namespace bea {

	Result<None> ModuleStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitModuleStatement(*this);
	}

	Result<None> PubStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitPubStatement(*this);
	}

	Result<None> UseStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitUseStatement(*this);
	}

	Result<None> LetStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitLetStatement(*this);
	}
}
