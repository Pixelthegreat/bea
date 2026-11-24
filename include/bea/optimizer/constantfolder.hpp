#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/context.hpp>
#include <bea/node/optimizer.hpp>

namespace bea {

	/*
	 * This optimizer is the constant folding stage
	 * of the compiler. It takes any expressions
	 * that can be known at compile time and
	 * simplifies them. For example:
	 *   1 + 2 * 3  ->  1 + 6  ->  7
	 */
	class ConstantFolder : public NodeOptimizerVisitor {
	public:
		ConstantFolder(Context& context)
			: m_context(context) {}

		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;
		virtual Result<None> visitUnaryOperation(UnaryOperationNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
		virtual Result<None> visitCast(CastNode& node) override;
	private:
		Context& m_context;
	};
}
