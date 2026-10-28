#pragma once

#include <bea/core.hpp>
#include <bea/serial.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * This analyzer declares global variables,
	 * functions and types and serializes them.
	 */
	class GlobalResolver : public NodeAnalyzerVisitor {
	public:
		GlobalResolver(
			Context& context,
			GRCodeSerializer& serializer
		)
			: NodeAnalyzerVisitor(context),
			  m_serializer(serializer) {}

		virtual Result<None> analyze(Node& node) override;

		virtual Result<None> visitBlock(BlockNode& node) override;
		virtual Result<None> visitUseStatement(UseStatementNode& node) override;

		Result<None> visitFuncStatement(FuncStatementNode& node);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
	private:
		GRCodeSerializer& m_serializer;
	};
}
