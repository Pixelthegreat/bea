/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/node/optimizer.hpp>

namespace bea {

	/*
	 * This optimizer does a very basic pass of
	 * simplication to the fresh AST.
	 */
	class PostParseSimplifier : public NodeOptimizerVisitor {
	public:
		virtual Result<None> visitRange(RangeNode& node) override;
	};
}
