#pragma once

#include <iostream>
#include <bea/core.hpp>
#include <bea/token.hpp>

namespace bea {

	/*
	 * Lexer
	 */
	class Lexer {
	public:
		Lexer(const char *path, std::ifstream& ifstream)
			: path(path), stream(ifstream) {}

		Result<std::vector<std::unique_ptr<Token>>&> makeTokens();
	private:
		const char *path;
		std::ifstream& stream;
		Position position;

		std::vector<std::unique_ptr<Token>> tokens;

		void advance();

		Result<None> makeNumber();
		Result<None> makeString();
		Result<None> makeIdentifier();
	};
}
