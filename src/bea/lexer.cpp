#include <format>
#include <bea/core.hpp>
#include <bea/lexer.hpp>

namespace bea {

	const char *Lexer::s_digit = "0123456789";
	const char *Lexer::s_identifier = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

	const char *Lexer::s_keywords[] = {
		"pub", "module", "use", "as",
		"abstract", "static", "class", "struct",
		"impl", "func",
		"let",
		"if", "elif", "else", "which", "case",
		"and", "or", "not",
		"for", "in", "while",
		"return", "continue", "break",
		"new", "cast",
		NULL,
	};

	/* Generate token stream */
	Result<None> Lexer::makeTokens() {

		advance();
		while (m_cc) {

			/* Whitespace */
			if (std::strchr(" \t\r", m_cc))
				advance();

			else if (std::strchr(";\n", m_cc)) {

				addToken(m_position, TokenType::EndLine, "");
				advance();
			}

			/* Comment or "/" */
			else if (m_cc == '/') {

				Position start = m_position;
				advance();

				if (m_cc == '/') {

					m_sstream << '/';
					while (!std::strchr("\n", m_cc)) {

						m_sstream << m_cc;
						advance();
					}
					commitToken(start, TokenType::Comment);
				}
				else if (m_cc == '*') {

					m_sstream << "/*";
					advance();

					while (m_cc) {

						char cc = m_cc;
						advance();

						m_sstream << cc;
						if (cc == '*' && m_cc == '/') {

							advance();
							m_sstream << '/';
							break;
						}
					}
					if (!m_cc) return Result<None>(
						m_position, "Unterminated '/*' comment"
					);
					commitToken(start, TokenType::Comment);
				}
				else {
					char ch[3] = {'/', 0, 0};
					if (m_cc == '=') {

						ch[1] = '=';
						advance();
					}
					addToken(start, TokenType::Symbol, ch);
				}
			}

			/* Number */
			else if (std::strchr(s_digit, m_cc))
				BEA_OK(None, makeNumber());

			/* Identifier */
			else if (std::strchr(s_identifier, m_cc))
				BEA_OK(None, makeIdentifier());

			/* String */
			else if (m_cc == '"')
				BEA_OK(None, makeString());

			/* Character */
			else if (m_cc == '\'')
				BEA_OK(None, makeCharacter());

			/* "+", "++", "+=", "-", "--", "-=" or "->" */
			else if (std::strchr("+-", m_cc)) {

				char ch[3] = {m_cc, 0, 0};

				Position start = m_position;
				advance();

				if (m_cc == ch[0] or m_cc == '=') {

					ch[1] = m_cc;
					advance();
				}
				else if (ch[0] == '-' and m_cc == '>') {

					ch[1] = '>';
					advance();
				}
				addToken(start, TokenType::Symbol, ch);
			}

			/* "<", ">", "<=", ">=", "<<", ">>", "<<=" or ">>=" */
			else if (std::strchr("<>", m_cc)) {

				char ch[4] = {m_cc, 0, 0, 0};
				size_t nch = 1;

				Position start = m_position;
				advance();

				if (m_cc == ch[0]) {

					ch[nch++] = m_cc;
					advance();
				}
				if (m_cc == '=') {

					ch[nch++] = '=';
					advance();
				}
				addToken(start, TokenType::Symbol, ch);
			}

			/* "=", "==" or "=>" */
			else if (m_cc == '=') {

				char ch[3] = {m_cc, 0, 0};

				Position start = m_position;
				advance();

				if (m_cc && std::strchr("=>", m_cc)) {

					ch[1] = m_cc;
					advance();
				}
				addToken(start, TokenType::Symbol, ch);
			}

			/* Could be followed by "=" */
			else if (std::strchr("*%|&^!", m_cc)) {

				char ch[3] = {m_cc, 0, 0};

				Position start = m_position;
				advance();

				if (m_cc == '=') {

					ch[1] = '=';
					advance();
				}
				addToken(start, TokenType::Symbol, ch);
			}

			/* Symbols that can be followed by themselves */
			else if (std::strchr(":.", m_cc)) {

				char ch[3] = {m_cc, 0, 0};

				Position start = m_position;
				advance();

				if (m_cc == ch[0]) {

					ch[1] = ch[0];
					advance();
				}
				addToken(start, TokenType::Symbol, ch);
			}

			/* Single characters */
			else if (std::strchr(",(){}[]?", m_cc)) {

				char ch[2] = {m_cc, 0};
				addToken(m_position, TokenType::Symbol, ch);
				advance();
			}

			/* single characters */
			else if (std::strchr(",(){}[]?", m_cc)) {

				Position start = m_position;
				m_sstream << m_cc;
				advance();

				commitToken(start, TokenType::Symbol);
			}

			/* Otherwise */
			else return Result<None>(m_position, std::format(
				"Unrecongized character '{}'", m_cc
			));
		}

		addToken(m_position, TokenType::EndFile, "");
		return Result<None>();
	}

	/* Advance lexer position */
	void Lexer::advance() {

		if (!m_cc) return;

		if (m_fstream.eof()) m_cc = 0;
		else m_cc = m_fstream.get();

		m_position.column++;
		if (m_cc == '\n') {

			m_position.line++;
			m_position.column = 0;
		}
		else if (m_cc == EOF)
			m_cc = 0;
	}

	/* Commit current token */
	void Lexer::commitToken(Position& position, TokenType type) {

		m_tokens.emplace_back(std::make_unique<Token>(
				position,
				type,
				m_sstream.str()
		));
		m_sstream.str(std::string(""));
	}

	/* Add token */
	void Lexer::addToken(Position& position, TokenType type, const char *value) {

		m_tokens.emplace_back(std::make_unique<Token>(
				position,
				type,
				value
		));
	}

	/* Generate number token */
	Result<None> Lexer::makeNumber() {

		Position start = m_position;
		TokenType type = TokenType::Integer;

		while (std::strchr(s_digit, m_cc)) {

			m_sstream << m_cc;
			advance();
		}
		if (m_cc == '.') {

			Position dot_start = m_position;
			advance();

			if (m_cc == '.') {

				advance();
				commitToken(start, TokenType::Integer);
				addToken(dot_start, TokenType::Symbol, "..");

				return Result<None>();
			}
			type = TokenType::Float;
			m_sstream << '.';

			while (std::strchr(s_digit, m_cc)) {

				m_sstream << m_cc;
				advance();
			}
		}
		commitToken(start, type);
		return Result<None>();
	}

	/* Generate string token */
	Result<None> Lexer::makeString() {

		Position start = m_position;
		advance();

		while (!std::strchr("\"", m_cc)) {

			char cc = m_cc;
			advance();

			if (cc == '\\') {

				cc = m_cc;
				advance();

				if (!cc) break;
				switch (cc) {
					case 'n': cc = '\n'; break;
					case 'r': cc = '\r'; break;
					case 't': cc = '\t'; break;
					case 'e': cc = '\x1b'; break;
				}
			}
			m_sstream << cc;
		}
		if (m_cc != '"')
			return Result<None>(
				m_position, "Expected '\"' at end of string literal"
			);
		advance();
		commitToken(start, TokenType::String);

		return Result<None>();
	}

	/* Generate character token */
	Result<None> Lexer::makeCharacter() {

		Position start = m_position;
		advance();

		while (!std::strchr("'", m_cc)) {

			char cc = m_cc;
			advance();

			if (cc == '\\') {

				cc = m_cc;
				advance();

				if (!cc) break;
				switch (cc) {
					case 'n': cc = '\n'; break;
					case 'r': cc = '\r'; break;
					case 't': cc = '\t'; break;
					case 'e': cc = '\x1b'; break;
				}
			}
			m_sstream << cc;
		}
		if (m_cc != '\'')
			return Result<None>(
				m_position, "Expected '`' at end of character literal"
			);
		else if (m_sstream.str().length() != 1)
			return Result<None>(
				m_position, "More or less than one character in character literal"
			);

		advance();
		commitToken(start, TokenType::Character);

		return Result<None>();
	}

	/* Generate identifier token */
	Result<None> Lexer::makeIdentifier() {

		Position start = m_position;
		while (m_cc && std::strchr(s_identifier, m_cc)) {

			m_sstream << m_cc;
			advance();
		}
		TokenType type = TokenType::Identifier;
		const std::string& str = m_sstream.str();

		for (size_t i = 0; s_keywords[i]; i++) {
			if (!std::strcmp(s_keywords[i], str.c_str())) {

				type = TokenType::Keyword;
				break;
			}
		}
		commitToken(start, type);
		return Result<None>();
	}
}
