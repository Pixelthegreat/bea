#include <cstring>
#include <iostream>
#include <fstream>
#include <format>
#include <filesystem>
#include <bea/core.hpp>
#include <bea/project.hpp>
#include <bea/application.hpp>

namespace bea {

	const char *Application::s_command = nullptr;
	const char *Application::s_projectPath = ".";
	Project::Target Application::s_target = Project::Target::Debug;

	/* Run application */
	Result<None> Application::run(int argc, const char **argv) {

		BEA_OK(None, parseArguments(argc, argv));

		if (!strcmp(s_command, "help")) {

			printHelp(argv[0]);
			return Result<None>();
		}

		/* Load project */
		Project *project = BEA_OK(
			None, Project::loadProject(
				s_projectPath,
				Project::defaultURL,
				s_target
			)
		);

		Result<None> result;

		if (!strcmp(s_command, "build")) {

			result = project->build();

			if (!result.okay) {

				std::cerr << *result.error << std::endl;
				std::cout << "\e[1;31mBuild unsuccessful\e[0m" << std::endl;
				result.error = nullptr;
			}
			else
				std::cout << "\e[1;32mBuild successful\e[0m" << std::endl;
		}
		else if (!strcmp(s_command, "run")) {

			result = project->build();

			if (!result.okay) {

				std::cerr << *result.error << std::endl;
				std::cout << "\e[1;31mBuild unsuccessful\e[0m" << std::endl;
				result.error = nullptr;
			}
			else {
				std::cout << "\e[1;32mBuild successful\e[0m" << std::endl;
				result = project->run();
			}
		}
		return result;
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
			if (arg[0] == '-') {

				if (!strcmp(arg, "-T") || !strcmp(arg, "--target")) {

					const char *target = argv[++i];
					if (!target) return Result<None>(std::format(
						"{}: Expected argument after '{}'",
						progname, arg
					));
					if (!strcmp(target, "debug"))
						s_target = Project::Target::Debug;
					else if (!strcmp(target, "release"))
						s_target = Project::Target::Release;
					else
						return Result<None>(std::format(
							"{}: Unrecognized target '{}'",
							progname, target
						));
				}
				else
					return Result<None>(std::format(
						"{}: Unrecognized option '{}'",
						progname, arg
					));
			}

			/* Command */
			else if (!s_command) s_command = arg;

			/* Project path */
			else if (!strcmp(s_projectPath, ".")) s_projectPath = arg;

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
			"    build [directory]  Build project\n"
			"    run [directory]    Run project\n";
	}
}
