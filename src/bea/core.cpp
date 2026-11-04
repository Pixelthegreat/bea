#include <bea/core.hpp>

namespace bea {

	/* Print error position */
	std::ostream& operator<<(std::ostream& stream, const Position& position) {

		return stream <<
			position.path << ":" <<
			position.line << "," <<
			position.column;
	}

	/* Print error */
	std::ostream& operator<<(std::ostream& stream, const Error& error) {

		stream << "\x1b[31mError\x1b[39m";

		if (error.position.path)
			stream <<
				" (File '" << error.position.path <<
				"', Line " << error.position.line <<
				", Column " << error.position.column <<
				"):\n  ";
		else stream << ": ";

		return stream << error.message;
	}
}
