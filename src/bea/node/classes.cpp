#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/classes.hpp>

namespace bea {

	Result<None> ClassFieldStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitClassFieldStatement(*this);
	}

	Result<None> ImplStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitImplStatement(*this);
	}

	Result<None> ClassStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitClassStatement(*this);
	}
}
