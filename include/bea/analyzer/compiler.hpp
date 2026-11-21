#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/context.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * Compiler
	 */
	class Compiler : public NodeAnalyzerVisitor {
	public:
		Compiler(
			Context& context,
			std::ostream& stream
		)
			: NodeAnalyzerVisitor(context),
			  m_stream(stream) {}
	private:
		std::ostream& m_stream;
	};
}
