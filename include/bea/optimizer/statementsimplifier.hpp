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
	 * This optimizer simplfies for loops and the ?
	 * operator. That's basically it. For example:
	 *   for i in 0..10 {
	 *   }
	 *   ->
	 *   for i in new Range(0, 10) {
	 *   }
	 *   ->
	 *   {
	 *     let __iter = cast<Iterable<T>>(new Range(0, 10)).iterator()
	 *     while not __iter.isAtEnd() {
	 *       ...
	 *     }
	 *   }
	 *
	 *   expr?
	 *   ->
	 *   {
	 *     let result = expr
	 *     if not result.okay {
	 *       return new Result<T>(result.error)
	 *     }
	 *     result.value
	 *   }
	 */
	class StatementSimplifier : public NodeOptimizerVisitor {
	public:
		StatementSimplifier(Context& context)
			: m_context(context) {}

		virtual Result<None> visitCarryResult(CarryResultNode& node) override;

		virtual Result<None> visitForStatement(ForStatementNode& node) override;

		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
	private:
		Context& m_context;

		TypeID *m_returnType = nullptr;
	};
}
