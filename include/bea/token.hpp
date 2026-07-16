#pragma once

#include <bea/core.hpp>

namespace bea {

	/*
	 * Token type
	 */
	enum TokenType {
		Integer = 0,
		Float,
		String,
		Symbol,
		Identifier,
		Keyword,

		Count,
	};

	/*
	 * Token
	 */
	struct Token {
		Position position;
		TokenType type;
		std::string value;

		Token(Position& position, TokenType type, std::string& value)
			: position(position), type(type), value(value) {}

		Token(Position& position, TokenType type, const char *value)
			: position(position), type(type), value(value) {}

		bool match(TokenType type, const char *value);

		bool matchKeyword(const char *value);

		bool matchSymbol(const char *value);
	};

	std::ostream& operator<<(std::ostream& stream, Token& token);
}
