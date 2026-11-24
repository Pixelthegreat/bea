/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/node/visitor.hpp>
#include <bea/node/functions.hpp>

namespace bea {

	Result<None> FuncExprStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitFuncExprStatement(*this);
	}

	Result<None> FuncBlockStatementNode::visit(NodeVisitor& visitor) {

		return visitor.visitFuncBlockStatement(*this);
	}
}
