#include <iostream>
#include <format>
#include <reproc/run.h>
#include <bea/core.hpp>
#include <bea/process.hpp>

namespace bea {

	/* Run process with arguments */
	Result<int> runProcess(
		std::vector<const char *>&& arguments,
		bool verbose
	) {
		arguments.push_back(nullptr);

		/* Print arguments */
		if (verbose) {

			std::size_t count = 0;
			for (const char *argument: arguments) {

				if (!argument) break;

				if (count++) std::cout << ' ';

				const char *space = std::strchr(argument, ' ');

				if (space) std::cout << '"';
				std::cout << argument;
				if (space) std::cout << '"';
			}
			std::cout << std::endl;
		}

		/* Run process */
		int code = reproc_run(
			arguments.data(),
			(reproc_options){0}
		);
		if (code < 0)
			return Result<int>(std::format(
				"Can't run '{}': {}",
				arguments[0],
				reproc_strerror(code)
			));
		else if (code)
			return Result<int>(std::format(
				"Execution failed (code {})",
				code
			));
		return Result<int>();
	}
}
