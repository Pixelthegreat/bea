/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>

namespace bea {

	/*
	 * Token type
	 */
	enum class TokenType {
		Integer = 0,
		Float,
		String,
		Character,
		Symbol,
		Identifier,
		Keyword,

		Comment,

		EndLine,
		EndFile,

		Count,
	};

	std::ostream& operator<<(std::ostream& stream, TokenType type);

	/*
	 * Token
	 */
	struct Token {
		Position position;
		TokenType type;
		std::string value;

		Token(Position& position, TokenType type, const std::string& value)
			: position(position), type(type), value(value) {}

		Token(Position& position, TokenType type, const char *value)
			: position(position), type(type), value(value) {}

		bool match(TokenType type, const char *value) const;

		bool matchKeyword(const char *value) const;

		bool matchSymbol(const char *value) const;
	};

	std::ostream& operator<<(std::ostream& stream, const Token& token);
}
