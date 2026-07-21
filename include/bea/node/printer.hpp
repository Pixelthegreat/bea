#pragma once

#include <bea/node.hpp>
#include <bea/node/visitor.hpp>

namespace bea {

	/*
	 * Node printer
	 */
	class NodePrinter : public NodeVisitor {
	public:
		NodePrinter(std::ostream& stream)
			: m_stream(stream) {}

		virtual Result<None> visitBlock(BlockNode& node) override;
		virtual Result<None> visitInteger(IntegerNode& node) override;
		virtual Result<None> visitFloat(FloatNode& node) override;
		virtual Result<None> visitString(StringNode& node) override;
		virtual Result<None> visitType(TypeNode& node) override;
		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;
		virtual Result<None> visitUnaryOperation(UnaryOperationNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
		virtual Result<None> visitCarryResult(CarryResultNode& node) override;
		virtual Result<None> visitNew(NewNode& node) override;
		virtual Result<None> visitCast(CastNode& node) override;
	private:
		std::ostream& m_stream;
		std::size_t m_tabLevel;
	};
}
