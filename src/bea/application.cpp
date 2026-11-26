/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
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
		Result<None> result;

		if (!std::strcmp(s_command, "help")) {

			printHelp(argv[0]);
			return Result<None>();
		}

		/* Create project */
		else if (!std::strcmp(s_command, "create"))
			BEA_OK(None, Project::createProject(s_projectPath));

		else if (!std::strcmp(s_command, "init"))
			BEA_OK(None, Project::createProject(
				/* s_projectPath here is used as the name instead */
				".", s_projectPath
			));

		/* Build project */
		else if (!std::strcmp(s_command, "build")) {

			Project *project = BEA_OK(
				None, Project::loadProject(
					s_projectPath,
					Project::s_defaultURL,
					s_target
				)
			);
			result = Project::buildAll();

			if (!result.okay) {

				std::cerr << *result.error << std::endl;
				std::cout << "\e[1;31mBuild unsuccessful\e[0m" << std::endl;
				result.error = nullptr;
			}
			else
				std::cout << "\e[1;32mBuild successful\e[0m" << std::endl;
		}

		/* Build and run project */
		else if (!std::strcmp(s_command, "run")) {

			Project *project = BEA_OK(
				None, Project::loadProject(
					s_projectPath,
					Project::s_defaultURL,
					s_target
				)
			);
			result = Project::buildAll();

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

		/* Error */
		else return Result<None>(std::format(
			"{}: Unrecognized command '{}'",
			argv[0], s_command
		));
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

				/* Target */
				if (!std::strcmp(arg, "-T") || !std::strcmp(arg, "--target")) {

					const char *target = argv[++i];
					if (!target) return Result<None>(std::format(
						"{}: Expected argument after '{}'",
						progname, arg
					));
					if (!std::strcmp(target, "debug"))
						s_target = Project::Target::Debug;
					else if (!std::strcmp(target, "release"))
						s_target = Project::Target::Release;
					else
						return Result<None>(std::format(
							"{}: Unrecognized target '{}'",
							progname, target
						));
				}

				/* Verbose command output */
				else if (!std::strcmp(arg, "-v") || !std::strcmp(arg, "--verbose"))
					Project::s_verbose = true;

				/* Build all */
				else if (!std::strcmp(arg, "-B") || !std::strcmp(arg, "--build-all"))
					Project::s_buildAll = true;

				/* Error */
				else
					return Result<None>(std::format(
						"{}: Unrecognized option '{}'",
						progname, arg
					));
			}

			/* Command */
			else if (!s_command) s_command = arg;

			/* Project path */
			else if (!std::strcmp(s_projectPath, ".")) s_projectPath = arg;

			/* Otherwise */
			else return Result<None>(std::format(
				"{}: Unexpected argument '{}'",
				progname, arg
			));
		}

		if (!s_command)
			return Result<None>(std::format(
				"{}: Expected command (See '{} help')",
				progname, progname
			));
		return Result<None>();
	}

	/* Print help string */
	void Application::printHelp(const char *progname) {

		std::cout <<
			"Usage: " << progname << " <command> ...\n"
			"\nCommands:\n"
			"    help                  Show this help message\n"
			"    init [name]           Initialize project in current directory\n"
			"    create [directory]    Create project in directory\n"
			"    build [directory]     Build project\n"
			"    run [directory]       Run project\n"
			"\nBuild options:\n"
			"    -B|--build-all        Build all sources\n"
			"    -T|--target <target>  Build target (debug or release)\n"
			"    -v|--verbose          Print verbose command output\n";
	}
}
