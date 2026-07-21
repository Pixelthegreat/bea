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

		Result<None> makeTokens();

		const std::vector<std::unique_ptr<Token>>& getTokens() const { return m_tokens; }
	private:
		const char *m_path;
		std::ifstream& m_fstream;
		Position m_position;
		std::stringstream m_sstream;
		char m_cc;

		std::vector<std::unique_ptr<Token>> m_tokens;

		void advance();
		void commitToken(Position& position, TokenType type);
		void addToken(Position& position, TokenType type, const char *value);

		Result<None> makeNumber();
		Result<None> makeString();
		Result<None> makeCharacter();
		Result<None> makeIdentifier();

		static const char *s_digit, *s_identifier;
		static const char *s_keywords[];
	};
}
