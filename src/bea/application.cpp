#include <cstring>
#include <iostream>
#include <format>
#include <bea/core.hpp>
#include <bea/type.hpp>
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

		bea::Type type(BaseType::Function);
		type.function.returnType = std::make_unique<bea::Type>(BaseType::Int);

		type.generics = bea::DynFixed<GenericInfo>(1);

		type.generics[0].type = bea::GenericType::Type;
		type.generics[0].name = "T";
		type.generics[0].substitutedType = std::make_unique<bea::Type>(BaseType::Int);

		type.function.parameters = bea::DynFixed<VariableInfo>(2);

		type.function.parameters[0].type = std::make_unique<bea::Type>(BaseType::Int);
		type.function.parameters[0].name = "x";
		type.function.parameters[1].type = std::make_unique<bea::Type>(BaseType::Int);
		type.function.parameters[1].name = "y";

		std::cout << type << std::endl;

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
