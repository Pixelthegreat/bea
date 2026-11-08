#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * This analyzer declares local variables,
	 * checks types and in general verifies that
	 * the code can be compiled.
	 */
	class LocalResolver : public NodeAnalyzerVisitor {
	public:
		LocalResolver(Context& context)
			: NodeAnalyzerVisitor(context) {}

		virtual Result<None> visitInteger(IntegerNode& node) override;
		virtual Result<None> visitFloat(FloatNode& node) override;
		virtual Result<None> visitString(StringNode& node) override;

		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
	private:
		/*
		 * Get type for type identifier
		 */
		std::unique_ptr<TypeID> registerTypeID(
			std::unique_ptr<TypeID> typeID
		);
	};
}
