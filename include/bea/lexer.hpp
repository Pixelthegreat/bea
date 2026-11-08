#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <bea/core.hpp>
#include <bea/token.hpp>

namespace bea {

	/*
	 * Lexer
	 */
	class Lexer {
	public:
		Lexer(const char *path, std::ifstream& ifstream)
			: m_path(path),
			  m_fstream(ifstream),
			  m_cc(1),
			  m_position{m_path, 1, 0} {}

		std::vector<std::unique_ptr<Token>> m_tokens;

		/*
		 * Create tokens
		 */
		Result<None> makeTokens();
	private:
		const char *m_path;
		std::ifstream& m_fstream;
		Position m_position;
		std::stringstream m_sstream;
		char m_cc;

		/*
		 * Advance position
		 */
		void advance();

		/*
		 * Add token from string stream
		 */
		void commitToken(Position& position, TokenType type);

		/*
		 * Add token
		 */
		void addToken(Position& position, TokenType type, const char *value);

		/*
		 * Create number token
		 */
		Result<None> makeNumber();

		/*
		 * Create string token
		 */
		Result<None> makeString();

		/*
		 * Create character token
		 */
		Result<None> makeCharacter();

		/*
		 * Create identifier or keyword token
		 */
		Result<None> makeIdentifier();

		static const char *s_digit, *s_identifier;
		static const char *s_keywords[];
	};
}
