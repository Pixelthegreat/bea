/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>
#include <bea/core.hpp>
#include <bea/application.hpp>

int main(int argc, const char **argv) {

	auto result = bea::Application::run(argc, argv);
	bea::Application::cleanUp();

	if (!result.okay) {

		if (result.error)
			std::cerr << *result.error << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
