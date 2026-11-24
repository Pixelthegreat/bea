/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/context.hpp>
#include <bea/node/optimizer.hpp>

namespace bea {

	/*
	 * This optimizer turns implicit conversions
	 * explict. For example:
	 *   let x: Float = 3  ->  let x: Float = cast<Float>(3)
	 */
	class ImplicitExpander : public NodeOptimizerVisitor {
	public:
		ImplicitExpander(Context& context)
			: m_context(context) {}

		virtual Result<None> visit(Node& node);

		virtual Result<None> visitUnaryOperation(UnaryOperationNode& node) override;
		virtual Result<None> visitAssignment(AssignmentNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
		virtual Result<None> visitNew(NewNode& node) override;
		virtual Result<None> visitList(ListNode& node) override;
		virtual Result<None> visitCall(CallNode& node) override;
		virtual Result<None> visitIndexedAccess(IndexedAccessNode& node) override;

		virtual Result<None> visitIfStatement(IfStatementNode& node) override;
		virtual Result<None> visitWhileStatement(WhileStatementNode& node) override;
		virtual Result<None> visitReturnStatement(ReturnStatementNode& node) override;

		virtual Result<None> visitLetStatement(LetStatementNode& node) override;
	private:
		Context& m_context;

		/*
		 * Make explicit cast of node
		 */
		Result<std::unique_ptr<Node>> makeExplicitCast(
			const TypeID& typeID,
			std::unique_ptr<Node> node
		);

		/*
		 * Make true cast of node
		 *
		 * x -> cast<True>(x).isTrue()
		 */
		std::unique_ptr<Node> makeTrueCast(
			std::unique_ptr<Node> node
		);
	};
}
