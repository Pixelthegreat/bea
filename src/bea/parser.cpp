#include <format>
#include <bea/core.hpp>
#include <bea/parser.hpp>

namespace bea {

	using NodeP = std::unique_ptr<Node>;

	/* Skip newlines, requiring at least one to be present */
	Parser::NoneResult Parser::skipNewlines() {

		if (m_token->type != TokenType::EndLine)
			return NoneResult(m_token->position, "Expected new line");

		skipNewlinesOpt();
		return NoneResult();
	}

	void Parser::skipNewlinesOpt() {

		while (m_token->type == TokenType::EndLine)
			advance();
	}

#define skipNewlines() BEA_OK(NodeP, skipNewlines())

	/* Parse file */
	Parser::Result Parser::parse() {

		advance();
		std::unique_ptr<BlockNode> node = std::make_unique<BlockNode>();

		/* Initial "module" statement */
		if (!m_token->matchKeyword("module"))
			return Result(
				m_token->position,
				std::format("Expected 'module' but got '{}'", m_token->value)
			);
		node->m_nodes.emplace_back(BEA_OK(NodeP, parseModuleStatement()));
		skipNewlines();

		/* Remaining toplevel statements */
		while (m_token->type != TokenType::EndFile) {

			if (m_token->matchKeyword("pub"))
				node->m_nodes.emplace_back(
					BEA_OK(NodeP, parsePubStatement())
				);
			else
				node->m_nodes.emplace_back(
					BEA_OK(NodeP, parseFileStatement())
				);
			skipNewlines();
		}
		return Result(std::move(node));
	}

	/* Advance to next token */
	void Parser::advanceRaw() {

		if (m_tokenIndex >= m_tokens.size())
			return;
		m_token = m_tokens[m_tokenIndex++].get();
	}

	void Parser::advance(Parser::Skip skipLevel) {

		while (true) {

			advanceRaw();
			if (!m_token) break;

			if (skipLevel == Skip::EndLine &&
			    m_token->type == TokenType::EndLine)
				continue;
			if (skipLevel >= Skip::Comment &&
			    m_token->type == TokenType::Comment)
				continue;

			break;
		}
	}

	/* Parse "module" statement */
	Parser::Result Parser::parseModuleStatement() {

		Position position = m_token->position;
		advance();

		Name name = BEA_OK(NodeP, parseName());

		auto node = std::make_unique<ModuleStatementNode>(std::move(name));
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse toplevel file statement */
	Parser::Result Parser::parseFileStatement() {

		if (m_token->matchKeyword("use"))
			return parseUseStatement();
		else if (m_token->matchKeyword("abstract") ||
			 m_token->matchKeyword("class") ||
			 m_token->matchKeyword("struct"))
			return parseClassStatement();
		else if (m_token->matchKeyword("func"))
			return parseFuncStatement();
		else if (m_token->matchKeyword("let"))
			return parseLetStatement();

		return Result(
			m_token->position,
			std::format("Unexpected token '{}'", m_token->value)
		);
	}

	/* Parse "use" statement */
	Parser::Result Parser::parseUseStatement() {

		Position position = m_token->position;
		advance();

		Name name = BEA_OK(NodeP, parseName());
		NodeP node = nullptr;

		/* "as" alias */
		if (m_token->matchKeyword("as")) {

			advance();

			Name alias = BEA_OK(NodeP, parseName());
			node = std::make_unique<UseStatementNode>(
				std::move(name), std::move(alias)
			);
		}
		else node = std::make_unique<UseStatementNode>(std::move(name));
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "class" statement */
	Parser::Result Parser::parseClassStatement() {

		Position position = m_token->position;
		advance();

		Name name = BEA_OK(NodeP, parseName());
		auto body = BEA_OK(NodeP, parseClassBlock());

		auto node = std::make_unique<ClassStatementNode>(
			std::move(name), std::move(body)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "pub" statement */
	Parser::Result Parser::parsePubStatement() {

		Position position = m_token->position;
		advance();

		NodeP statement = BEA_OK(NodeP, parseFileStatement());

		auto node = std::make_unique<PubStatementNode>(std::move(statement));
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse block of in-class statements */
	Parser::Result Parser::parseClassBlock() {

		if (!m_token->matchSymbol("{"))
			return Result(
				m_token->position,
				std::format(
					"Expected '{{' after class declaration but got '{}'",
					m_token->value
				)
			);
		advance();
		skipNewlines();

		auto node = std::make_unique<BlockNode>();

		while (!m_token->matchSymbol("}")) {

			if (m_token->matchKeyword("pub"))
				node->m_nodes.emplace_back(
					BEA_OK(NodeP, parseClassPubStatement())
				);
			else if (m_token->matchKeyword("impl"))
				node->m_nodes.emplace_back(
					BEA_OK(NodeP, parseClassImplStatement())
				);
			else
				node->m_nodes.emplace_back(
					BEA_OK(NodeP, parseClassBlockStatement())
				);
			skipNewlines();
		}
		advance();

		return Result(std::move(node));
	}

	/* Parse in-class statement */
	Parser::Result Parser::parseClassBlockStatement() {

		if (m_token->matchKeyword("func") ||
		    m_token->matchKeyword("abstract"))
			return parseFuncStatement();

		else if (m_token->type == TokenType::Identifier)
			return parseClassField();

		return Result(
			m_token->position,
			std::format("Unexpected token '{}'", m_token->value)
		);
	}

	/* Parse in-class "pub" statement */
	/* TODO: Add dynamic cast and m_public = true */
	Parser::Result Parser::parseClassPubStatement() {

		Position position = m_token->position;
		advance();

		NodeP statement = BEA_OK(NodeP, parseClassBlockStatement());

		auto node = std::make_unique<PubStatementNode>(std::move(statement));
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse in-class "impl" statement */
	Parser::Result Parser::parseClassImplStatement() {

		Position position = m_token->position;
		advance();

		NodeP type = BEA_OK(NodeP, parseType());
		NodeP body = BEA_OK(NodeP, parseClassImplBlock());

		auto node = std::make_unique<ImplStatementNode>(
			std::move(type), std::move(body)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse block of in-impl statements */
	Parser::Result Parser::parseClassImplBlock() {

		if (!m_token->matchSymbol("{"))
			return Result(
				m_token->position,
				std::format(
					"Expected '{{' after impl declaration but got '{}'",
					m_token->value
				)
			);
		advance();
		skipNewlines();

		auto node = std::make_unique<BlockNode>();

		while (!m_token->matchSymbol("}")) {

			node->m_nodes.emplace_back(
				BEA_OK(NodeP, parseClassBlockStatement())
			);
			skipNewlines();
		}
		advance();

		return Result(std::move(node));
	}

	/* Parse in-class field declaration */
	Parser::Result Parser::parseClassField() {

		Position position = m_token->position;

		Name name = BEA_OK(NodeP, parseName());

		if (!m_token->matchSymbol(":"))
			return Result(
				m_token->position,
				std::format(
					"Expected ':' after name but got '{}'",
					m_token->value
				)
			);
		advance();
		NodeP type = BEA_OK(NodeP, parseType());
		NodeP value = nullptr;

		if (m_token->matchSymbol("=")) {

			advance();
			value = BEA_OK(NodeP, parseExpr());
		}
		auto node = std::make_unique<ClassFieldStatementNode>(
			std::move(name), std::move(type), std::move(value)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse type */
	Parser::Result Parser::parseType() {

		Position position = m_token->position;

		Name name = BEA_OK(NodeP, parseName());
		auto node = std::make_unique<TypeNode>(std::move(name));

		node->m_position = position;

		/* Generic arguments */
		if (m_token->matchSymbol("<")) {

			advance(Skip::EndLine);

			bool start = true;
			while (start || m_token->matchSymbol(",")) {

				if (!start) advance(Skip::EndLine);
				start = false;

				/* Integer argument */
				if (m_token->type == TokenType::Integer) {

					std::intmax_t value;
					m_sstream.str(m_token->value);
					m_sstream >> value;

					advance();

					node->m_genericArgumentNodes.emplace_back(
						std::make_unique<IntegerNode>(value)
					);
				}
				else node->m_genericArgumentNodes.emplace_back(
						BEA_OK(NodeP, parseType())
				);
			}
			skipNewlinesOpt();

			if (!m_token->matchSymbol(">")) return Result(
				m_token->position,
				std::format(
					"Expected '>' after generic arguments but got '{}'",
					m_token->value
				)
			);
			advance(Skip::EndLine);
		}
		return Result(std::move(node));
	}

	/* Parse name */
	Parser::NameResult Parser::parseName() {

		if (m_token->type != TokenType::Identifier)
			return NameResult(
				m_token->position,
				std::format("Expected name but got '{}'", m_token->value)
			);
		std::array<const Token *, 32> tokens{m_token};
		advance();

		/* Add any extra name components (A::B, etc) */
		std::size_t count = 1;
		while (m_token->matchSymbol("::")) {

			if (count >= tokens.size())
				return NameResult(
					m_token->position,
					"Too many name components"
				);

			advance();
			if (m_token->type != TokenType::Identifier)
				return NameResult(
					m_token->position,
					std::format(
						"Expected name component but got '{}'",
						m_token->value
					)
				);
			tokens[count++] = m_token;
			advance();
		}

		/* Create name */
		Name name(count);

		for (std::size_t i = 0; i < count; i++)
			name.components[i] = tokens[i]->value;

		return NameResult(name);
	}
}
