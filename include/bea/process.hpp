/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>

namespace bea {

	/*
	 * Run process with arguments
	 */
	Result<int> runProcess(
		std::vector<const char *>&& arguments,
		bool verbose = false
	);
}
