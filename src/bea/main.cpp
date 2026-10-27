#include <iostream>
#include <bea/core.hpp>
#include <bea/application.hpp>

int main(int argc, const char **argv) {

	auto result = bea::Application::run(argc, argv);
	bea::Application::cleanUp();

	if (!result.okay && result.error) {

		std::cerr << *result.error << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
