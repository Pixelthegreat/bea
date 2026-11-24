/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/optimizer/postparsesimplifier.hpp>

namespace bea {

	/*
	 * Visit range
	 *
	 * start..end -> new Range(start, end)
	 */
	Result<None> PostParseSimplifier::visitRange(RangeNode& node) {

		auto typeNode = std::make_unique<TypeNode>("Range");
		typeNode->m_position = node.m_position;

		auto newNode = std::make_unique<NewNode>(std::move(typeNode));
		newNode->m_position = node.m_position;

		newNode->m_argumentNodes.emplace_back(std::move(node.m_startNode));
		newNode->m_argumentNodes.emplace_back(std::move(node.m_endNode));

		m_optimizedNodes.emplace_back(std::move(newNode));
		return Mode::Replace;
	}
};
