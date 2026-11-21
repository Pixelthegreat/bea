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
