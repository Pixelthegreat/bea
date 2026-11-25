/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/token.hpp>

namespace bea {

	/* Output type to stream */
	std::ostream& operator<<(std::ostream& stream, TokenType type) {
		switch (type) {
			using enum TokenType;
			case Integer: return stream << "Integer";
			case Float: return stream << "Float";
			case String: return stream << "String";
			case Character: return stream << "Character";
			case Symbol: return stream << "Symbol";
			case Identifier: return stream << "Identifier";
			case Keyword: return stream << "Keyword";
			case Attribute: return stream << "Attribute";
			case Comment: return stream << "Comment";
			case EndLine: return stream << "EndLine";
			case EndFile: return stream << "EndFile";
		}
		return stream;
	}

	/* Match against a type and value */
	bool Token::match(TokenType type, const char *value) const {

		return this->type == type and this->value == value;
	}

	/* Match against a keyword value */
	bool Token::matchKeyword(const char *value) const {

		return match(TokenType::Keyword, value);
	}

	/* Match against a symbol value */
	bool Token::matchSymbol(const char *value) const {

		return match(TokenType::Symbol, value);
	}

	/* Match against an attribute value */
	bool Token::matchAttribute(const char *value) const {

		return match(TokenType::Attribute, value);
	}

	/* Output to stream */
	std::ostream& operator<<(std::ostream& stream, const Token& token) {

		return stream << token.type << ":'" << token.value << "'";
	}
}
