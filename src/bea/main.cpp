#include <iostream>
#include <bea/core.hpp>
#include <bea/application.hpp>

int main(int argc, const char **argv) {

	auto result = bea::Application::run(argc, argv);
	bea::Application::cleanUp();

	if (!result.okay) {

		std::cerr << *result.error.get() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
