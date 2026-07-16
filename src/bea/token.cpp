#include <bea/core.hpp>
#include <bea/token.hpp>

namespace bea {

	/* match against a type and value */
	bool Token::match(TokenType type, const char *value) {

		return this->type == type and this->value == value;
	}

	/* match against a keyword value */
	bool Token::matchKeyword(const char *value) {

		return match(TokenType::Keyword, value);
	}

	/* match against a symbol value */
	bool Token::matchSymbol(const char *value) {

		return match(TokenType::Symbol, value);
	}

	/* output to stream */
	std::ostream& operator<<(std::ostream& stream, Token& token) {

		return stream << token.type << ":" << token.value;
	}
}
