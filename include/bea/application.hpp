#pragma once

#include <bea/core.hpp>

namespace bea {

	/*
	 * Application
	 */
	class Application {
	public:
		static Result<None> run(int argc, const char **argv);
		static void cleanUp();
	private:
		static const char *s_command;

		static Result<None> parseArguments(int argc, const char **argv);
		static void printHelp(const char *progname);
	};
}
