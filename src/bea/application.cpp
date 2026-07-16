#include <cstring>
#include <iostream>
#include <format>
#include <bea/core.hpp>
#include <bea/token.hpp>
#include <bea/application.hpp>

namespace bea {

	const char *Application::s_command = nullptr;

	/* Run application */
	Result<None> Application::run(int argc, const char **argv) {

		BEA_OK(None, parseArguments(argc, argv));

		if (!strcmp(s_command, "help")) {

			printHelp(argv[0]);
			return Result<None>();
		}
		Position p{};
		Token t = Token(p, TokenType::Keyword, "Hello, world!");

		std::cout << t << std::endl;

		return Result<None>();
	}

	/* Destroy application */
	void Application::cleanUp() {
	}

	/* Parse arguments */
	Result<None> Application::parseArguments(int argc, const char **argv) {

		const char *progname = argv[0];
		for (int i = 1; i < argc; i++) {

			/* Option */
			const char *arg = argv[i];
			if (arg[0] == '-')
				return Result<None>(std::format(
					"{}: Unrecognized option '{}'",
					progname, arg
				));

			/* Command */
			else if (!s_command) s_command = arg;

			/* Otherwise */
			else return Result<None>(std::format(
					"{}: Unexpected argument '{}'",
					progname, arg
			));
		}

		if (!s_command)
			return Result<None>(std::format(
					"{}: Expected command argument",
					progname
			));
		return Result<None>();
	}

	/* Print help string */
	void Application::printHelp(const char *progname) {

		std::cout <<
			"Usage: " << progname << " <command> ...\n"
			"\nCommands:\n"
			"    help               Show this help message\n"
			"    build <directory>  Build project\n"
			"    run <directory>    Run project\n";
	}
}
