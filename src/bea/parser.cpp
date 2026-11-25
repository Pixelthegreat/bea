/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <format>
#include <bea/core.hpp>
#include <bea/parser.hpp>

namespace bea {

	using NodeP = std::unique_ptr<Node>;

	/* Skip newlines */
	void Parser::skipNewlines() {

		while (m_token->type == TokenType::EndLine)
			advance();
	}

	/* Match token to match list */
	bool Parser::matchTokenList(
		const TokenMatchList& pairs,
		const Token *token
	) {
		for (const std::tuple<TokenType, const char *>& pair: pairs) {
			if (token->type == std::get<0>(pair) &&
			    token->value == std::get<1>(pair))
				return true;
		}
		return false;
	}

	/* Parse binary operation */
	Parser::Result Parser::parseBinaryOperation(
		std::function<Parser::Result(Parser& parser)> leftFunc,
		std::function<Parser::Result(Parser& parser)> rightFunc,
		const TokenMatchList& pairs,
		std::size_t iterations
	) {
		NodeP left = BEA_OK(NodeP, leftFunc(*this));
		for (; matchTokenList(pairs, m_token) && iterations != 0; iterations--) {

			Position position = m_token->position;
			const std::string& operation = m_token->value;
			advance();

			NodeP right = BEA_OK(NodeP, rightFunc(*this));

			left = std::make_unique<BinaryOperationNode>(
				operation, std::move(left), std::move(right)
			);
			left->m_position = position;
		}
		return Result(std::move(left));
	}

	/* Parse file */
	Parser::Result Parser::parse() {

		advance(Skip::EndLine);

		std::unique_ptr<BlockNode> node = std::make_unique<BlockNode>();
		node->m_position = m_token->position;

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

		if (m_token->type == TokenType::Comment)
			m_lastCommentToken = m_token;
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

		const Token *commentToken = m_lastCommentToken;
		m_lastCommentToken = nullptr;

		bool abstract = false;
		if (m_token->matchKeyword("abstract")) {

			advance();
			abstract = true;

			if (!m_token->matchKeyword("class") &&
			    !m_token->matchKeyword("struct"))
				return Result(
					m_token->position,
					std::format(
						"Expected 'class' or 'struct' after 'abstract' but got '{}'",
						m_token->value
					)
				);
		}

		Position position = m_token->position;
		const std::string& kind = m_token->value;

		advance();

		Name name = BEA_OK(NodeP, parseName());

		/* Parse generic parameters */
		std::vector<NodeGenericParameter> genericParameters;
		if (m_token->matchSymbol("<")) {

			advance(Skip::EndLine);

			genericParameters.emplace_back(
				BEA_OK(NodeP, parseGenericParameter())
			);
			while (m_token->matchSymbol(",")) {

				advance();

				genericParameters.emplace_back(
					BEA_OK(NodeP, parseGenericParameter())
				);
			}
			if (!m_token->matchSymbol(">")) {

				return Result(
					m_token->position,
					std::format(
						"Expected '>' after generic parameters but got '{}'",
						m_token->value
					)
				);
			}
			advance();
		}

		/* Body */
		auto body = BEA_OK(NodeP, parseClassBlock());

		auto node = std::make_unique<ClassStatementNode>(
			std::move(name), std::move(body)
		);
		node->m_position = position;
		node->m_abstract = abstract;
		node->m_genericParameters = std::move(genericParameters);
		node->m_commentToken = commentToken;

		if (kind == "struct")
			node->m_copy = true;

		return Result(std::move(node));
	}

	/* Parse "func" statement */
	Parser::Result Parser::parseFuncStatement() {

		const Token *commentToken = m_lastCommentToken;
		m_lastCommentToken = nullptr;

		bool isStatic = false, isAbstract = false;

		if (m_token->matchKeyword("abstract")) {

			advance();
			isAbstract = true;
		}
		if (m_token->matchKeyword("static")) {

			advance();
			isStatic = true;
		}
		if (!m_token->matchKeyword("func"))
			return Result(
				m_token->position,
				std::format(
					"Expected 'func' but got '{}'",
					m_token->value
				)
			);
		Position position = m_token->position;
		advance();

		Name name = BEA_OK(NodeP, parseName());

		/* Parse generic parameters */
		std::vector<NodeGenericParameter> genericParameters;
		if (m_token->matchSymbol("<")) {

			advance(Skip::EndLine);

			genericParameters.emplace_back(
				BEA_OK(NodeP, parseGenericParameter())
			);
			while (m_token->matchSymbol(",")) {

				advance();

				genericParameters.emplace_back(
					BEA_OK(NodeP, parseGenericParameter())
				);
			}
			if (!m_token->matchSymbol(">")) {

				return Result(
					m_token->position,
					std::format(
						"Expected '>' after generic parameters but got '{}'",
						m_token->value
					)
				);
			}
			advance();
		}

		/* Parse parameters */
		std::vector<NodeFuncParameter> parameters;
		if (!m_token->matchSymbol("(")) {

			return Result(
				m_token->position,
				std::format(
					"Expected '(' after function name but got '{}'",
					m_token->value
				)
			);
		}
		advance(Skip::EndLine);

		if (!m_token->matchSymbol(")")) {

			parameters.emplace_back(
				BEA_OK(NodeP, parseFuncParameter())
			);
			while (m_token->matchSymbol(",")) {

				advance(Skip::EndLine);

				parameters.emplace_back(
					BEA_OK(NodeP, parseFuncParameter())
				);
			}
		}
		skipNewlines();

		if (!m_token->matchSymbol(")")) {

			return Result(
				m_token->position,
				std::format(
					"Expected ')' after function parameters but got '{}'",
					m_token->value
				)
			);
		}
		advance(Skip::EndLine);

		/* Return value */
		NodeP returnTypeNode = nullptr;
		if (m_token->matchSymbol("->")) {

			advance(Skip::EndLine);
			returnTypeNode = BEA_OK(NodeP, parseType());
		}
		else {
			returnTypeNode = std::make_unique<TypeNode>("None");
			returnTypeNode->m_position = m_token->position;
		}

		/* Expression body */
		if (m_token->matchSymbol("=>")) {

			advance(Skip::EndLine);
			NodeP bodyNode = BEA_OK(NodeP, parseExpr());

			auto node = std::make_unique<FuncExprStatementNode>(
				std::move(name), std::move(returnTypeNode),
				std::move(bodyNode)
			);
			node->m_position = position;
			node->m_genericParameters = std::move(genericParameters);
			node->m_parameters = std::move(parameters);
			node->m_commentToken = commentToken;

			return Result(std::move(node));
		}

		/* Block body */
		bool useDefault = false;

		if (m_token->matchSymbol("!")) {

			advance(Skip::EndLine);
			useDefault = true;
		}

		auto bodyNode = std::make_unique<BlockNode>();
		bool hollow = false;

		if (m_token->matchSymbol("{"))
			BEA_OK(NodeP, parseBlock(*bodyNode));

		else if (m_token->matchAttribute("#hollow")) {

			hollow = true;
			advance();
		}
		auto node = std::make_unique<FuncBlockStatementNode>(
			std::move(name),
			std::move(returnTypeNode),
			std::move(bodyNode),
			hollow
		);
		node->m_position = position;
		node->m_abstract = isAbstract;
		node->m_static = isStatic;
		node->m_useDefault = useDefault;
		node->m_genericParameters = std::move(genericParameters);
		node->m_parameters = std::move(parameters);
		node->m_commentToken = commentToken;

		return Result(std::move(node));
	}

	/* Parse "let" statement */
	Parser::Result Parser::parseLetStatement() {

		const Token *commentToken = m_lastCommentToken;
		m_lastCommentToken = nullptr;

		Position position = m_token->position;
		advance();

		std::vector<Name> names = {
			BEA_OK(NodeP, parseName())
		};
		while (m_token->matchSymbol(",")) {

			advance();

			names.emplace_back(
				BEA_OK(NodeP, parseName())
			);
		}

		/* Variable type */
		NodeP typeNode = nullptr;
		if (m_token->matchSymbol(":")) {

			advance();
			typeNode = BEA_OK(NodeP, parseType());
		}

		/* Variable value */
		NodeP valueNode = nullptr;
		if (m_token->matchSymbol("=")) {

			advance();
			valueNode = BEA_OK(NodeP, parseExpr());
		}

		auto node = std::make_unique<LetStatementNode>(
			names, std::move(typeNode), std::move(valueNode)
		);
		node->m_position = position;
		node->m_commentToken = commentToken;

		return Result(std::move(node));
	}

	/* Parse "pub" statement */
	Parser::Result Parser::parsePubStatement() {

		Position position = m_token->position;
		advance();

		NodeP statement = BEA_OK(NodeP, parseFileStatement());

		ExportableNode *statementP =
			dynamic_cast<ExportableNode *>(statement.get());
		if (statementP) statementP->m_public = true;

		return Result(std::move(statement));
	}

	/* Parse block of in-class statements */
	Parser::Result Parser::parseClassBlock() {

		Position position = m_token->position;

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
		node->m_position = position;

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
		    m_token->matchKeyword("abstract") ||
		    m_token->matchKeyword("static"))
			return parseFuncStatement();

		else if (m_token->type == TokenType::Identifier)
			return parseClassField();

		return Result(
			m_token->position,
			std::format("Unexpected token '{}'", m_token->value)
		);
	}

	/* Parse in-class "pub" statement */
	Parser::Result Parser::parseClassPubStatement() {

		Position position = m_token->position;
		advance();

		NodeP statement = BEA_OK(NodeP, parseClassBlockStatement());

		ExportableNode *statementP =
			dynamic_cast<ExportableNode *>(statement.get());
		if (statementP) statementP->m_public = true;

		return Result(std::move(statement));
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

		Position position = m_token->position;

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
		node->m_position = position;

		while (!m_token->matchSymbol("}")) {

			if (!m_token->matchKeyword("func") &&
			    !m_token->matchKeyword("static"))
				return Result(
					m_token->position,
					std::format(
						"Expected 'func' in impl block but got '{}'",
						m_token->value
					)
				);

			node->m_nodes.emplace_back(
				BEA_OK(NodeP, parseFuncStatement())
			);
			skipNewlines();
		}
		advance();

		return Result(std::move(node));
	}

	/* Parse in-class field declaration */
	Parser::Result Parser::parseClassField() {

		const Token *commentToken = m_lastCommentToken;
		m_lastCommentToken = nullptr;

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
		node->m_commentToken = commentToken;

		return Result(std::move(node));
	}

	/* Parse block */
	Parser::NoneResult Parser::parseBlock(BlockNode& block) {

		block.m_position = m_token->position;

		if (!m_token->matchSymbol("{"))
			return NoneResult(
				m_token->position,
				std::format(
					"Expected '{{' but got '{}'",
					m_token->value
				)
			);

		advance();
		skipNewlines();

		while (!m_token->matchSymbol("}")) {

			block.m_nodes.emplace_back(
				BEA_OK(None, parseStatement())
			);
			skipNewlines();
		}
		advance();
		return NoneResult();
	}

	/* Parse statement */
	Parser::Result Parser::parseStatement() {

		if (m_token->matchKeyword("let"))
			return parseLetStatement();

		else if (m_token->matchKeyword("if"))
			return parseIfStatement();

		else if (m_token->matchKeyword("which"))
			return parseWhichStatement();

		else if (m_token->matchKeyword("for"))
			return parseForStatement();

		else if (m_token->matchKeyword("while"))
			return parseWhileStatement();

		else if (m_token->matchKeyword("return"))
			return parseReturnStatement();

		else if (m_token->matchKeyword("continue"))
			return parseContinueStatement();

		else if (m_token->matchKeyword("break"))
			return parseBreakStatement();

		return parseExpr();
	}

	/* Parse "if" statement */
	Parser::Result Parser::parseIfStatement() {

		Position position = m_token->position;
		advance();

		NodeP condition = BEA_OK(NodeP, parseExpr());

		auto block = std::make_unique<BlockNode>();
		BEA_OK(NodeP, parseBlock(*block));

		std::vector<Pair<NodeP>> conditionalClauseNodes;

		conditionalClauseNodes.emplace_back(Pair<NodeP>{
			std::move(condition), std::move(block),
		});

		/* Secondary clauses */
		while (m_token->matchKeyword("elif")) {

			advance();

			condition = BEA_OK(NodeP, parseExpr());

			block = std::make_unique<BlockNode>();
			BEA_OK(NodeP, parseBlock(*block));

			conditionalClauseNodes.emplace_back(Pair<NodeP>{
				std::move(condition), std::move(block),
			});
		}

		/* Else clause */
		std::unique_ptr<BlockNode> elseClauseNode = nullptr;
		if (m_token->matchKeyword("else")) {

			advance();

			elseClauseNode = std::make_unique<BlockNode>();
			BEA_OK(NodeP, parseBlock(*elseClauseNode));
		}

		auto node = std::make_unique<IfStatementNode>(
			std::move(elseClauseNode)
		);
		node->m_position = position;
		node->m_conditionalClauseNodes = std::move(conditionalClauseNodes);

		return Result(std::move(node));
	}

	/* Parse "which" statement */
	Parser::Result Parser::parseWhichStatement() {

		Position position = m_token->position;
		advance();

		NodeP valueNode = BEA_OK(NodeP, parseExpr());
		NodeP bodyNode = BEA_OK(NodeP, parseWhichBlock());

		auto node = std::make_unique<WhichStatementNode>(
			std::move(valueNode), std::move(bodyNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "which" block */
	Parser::Result Parser::parseWhichBlock() {

		Position position = m_token->position;

		auto node = std::make_unique<BlockNode>();
		node->m_position = position;

		if (!m_token->matchSymbol("{"))
			return Result(
				m_token->position,
				std::format(
					"Expected '{{' after 'which' statement but got '{}'",
					m_token->value
				)
			);

		advance();
		skipNewlines();

		while (!m_token->matchSymbol("}")) {

			node->m_nodes.emplace_back(
				BEA_OK(NodeP, parseWhichBlockStatement())
			);
			skipNewlines();
		}
		advance();

		return Result(std::move(node));
	}

	/* Parse "case" or "case let" statement */
	Parser::Result Parser::parseWhichBlockStatement() {

		Position position = m_token->position;

		if (!m_token->matchKeyword("case"))
			return Result(
				m_token->position,
				std::format(
					"Expected 'case' in 'which' block but got '{}'",
					m_token->value
				)
			);
		advance();

		/* "case let" statement */
		if (m_token->matchKeyword("let")) {

			advance();

			Name name = BEA_OK(NodeP, parseName());

			if (!m_token->matchSymbol("="))
				return Result(
					m_token->position,
					std::format(
						"Expected '=' after variable name but got '{}'",
						m_token->value
					)
				);
			advance();

			NodeP valueNode = BEA_OK(NodeP, parseExpr());

			/* Statement body */
			NodeP bodyNode = nullptr;
			if (m_token->matchSymbol("->")) {

				advance();
				bodyNode = BEA_OK(NodeP, parseStatement());
			}
			else {
				auto bodyNodeP = std::make_unique<BlockNode>();
				BEA_OK(NodeP, parseBlock(*bodyNodeP));

				bodyNode = std::move(bodyNodeP);
			}

			auto node = std::make_unique<CaseLetStatementNode>(
				std::move(name), std::move(valueNode), std::move(bodyNode)
			);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Regular "case" statement */
		NodeP valueNode = nullptr;
		if (m_token->matchSymbol("_"))
			advance();
		else
			valueNode = BEA_OK(NodeP, parseExpr());

		NodeP bodyNode = nullptr;
		if (m_token->matchSymbol("->")) {

			advance();
			bodyNode = BEA_OK(NodeP, parseStatement());
		}
		else {
			auto bodyNodeP = std::make_unique<BlockNode>();
			BEA_OK(NodeP, parseBlock(*bodyNodeP));

			bodyNode = std::move(bodyNodeP);
		}

		auto node = std::make_unique<CaseStatementNode>(
			std::move(valueNode), std::move(bodyNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "for" statement */
	Parser::Result Parser::parseForStatement() {

		Position position = m_token->position;
		advance();

		Name name = BEA_OK(NodeP, parseName());

		if (!m_token->matchKeyword("in"))
			return Result(
				m_token->position,
				std::format(
					"Expected 'in' after variable name but got '{}'",
					m_token->value
				)
			);
		advance();

		NodeP iterableNode = BEA_OK(NodeP, parseExpr());

		auto bodyNode = std::make_unique<BlockNode>();
		BEA_OK(NodeP, parseBlock(*bodyNode));

		auto node = std::make_unique<ForStatementNode>(
			std::move(name), std::move(iterableNode), std::move(bodyNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "while" statement */
	Parser::Result Parser::parseWhileStatement() {

		Position position = m_token->position;
		advance();

		NodeP conditionNode = BEA_OK(NodeP, parseExpr());

		auto bodyNode = std::make_unique<BlockNode>();
		BEA_OK(NodeP, parseBlock(*bodyNode));

		auto node = std::make_unique<WhileStatementNode>(
			std::move(conditionNode), std::move(bodyNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "return" statement */
	Parser::Result Parser::parseReturnStatement() {

		Position position = m_token->position;
		advance();

		NodeP valueNode = nullptr;
		if (m_token->type < TokenType::EndLine)
			valueNode = BEA_OK(NodeP, parseExpr());
		else {
			valueNode = std::make_unique<VariableAccessNode>("None");
			valueNode->m_position = position;
		}

		auto node = std::make_unique<ReturnStatementNode>(
			std::move(valueNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "continue" statement */
	Parser::Result Parser::parseContinueStatement() {

		Position position = m_token->position;
		advance();

		auto node = std::make_unique<ContinueStatementNode>();
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse "break" statement */
	Parser::Result Parser::parseBreakStatement() {

		Position position = m_token->position;
		advance();

		auto node = std::make_unique<BreakStatementNode>();
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse expression */
	Parser::Result Parser::parseExpr() {

		static const TokenMatchList s_exprTokens = {
			{TokenType::Symbol, "="},
			{TokenType::Symbol, "+="},
			{TokenType::Symbol, "-="},
			{TokenType::Symbol, "*="},
			{TokenType::Symbol, "/="},
			{TokenType::Symbol, "%="},
			{TokenType::Symbol, "|="},
			{TokenType::Symbol, "&="},
			{TokenType::Symbol, "^="},
			{TokenType::Symbol, "<<="},
			{TokenType::Symbol, ">>="},
		};
		NodeP left = BEA_OK(NodeP, parseAssignExpr());
		if (matchTokenList(s_exprTokens, m_token)) {

			Position position = m_token->position;
			const std::string& operation = m_token->value;
			advance();

			NodeP right = BEA_OK(NodeP, parseExpr());

			left = std::make_unique<AssignmentNode>(
				operation, std::move(left), std::move(right)
			);
			left->m_position = position;
		}
		return Result(std::move(left));
	}

	/* Parse assign expression */
	Parser::Result Parser::parseAssignExpr() {

		static const TokenMatchList s_assignExprTokens = {
			{TokenType::Keyword, "and"},
			{TokenType::Keyword, "or"},
		};
		return parseBinaryOperation(
			&Parser::parseCompExpr,
			&Parser::parseCompExpr,
			s_assignExprTokens
		);
	}

	/* Parse comparison expression */
	Parser::Result Parser::parseCompExpr() {

		if (m_token->matchKeyword("not")) {

			Position position = m_token->position;
			const std::string& operation = m_token->value;

			advance();

			NodeP compExpr = BEA_OK(NodeP, parseCompExpr());

			auto node = std::make_unique<UnaryOperationNode>(
				operation, std::move(compExpr)
			);
			node->m_position = position;

			return Result(std::move(node));
		}

		static const TokenMatchList s_compExprTokens = {
			{TokenType::Symbol, "=="},
			{TokenType::Symbol, "!="},
			{TokenType::Symbol, "<"},
			{TokenType::Symbol, ">"},
			{TokenType::Symbol, "<="},
			{TokenType::Symbol, ">="},
		};
		return parseBinaryOperation(
			&Parser::parseArithExpr,
			&Parser::parseArithExpr,
			s_compExprTokens
		);
	}

	/* Parse arithmetic expression */
	Parser::Result Parser::parseArithExpr() {

		static const TokenMatchList s_arithExprTokens = {
			{TokenType::Symbol, "+"},
			{TokenType::Symbol, "-"},
			{TokenType::Symbol, "|"},
			{TokenType::Symbol, "&"},
			{TokenType::Symbol, "^"},
		};
		return parseBinaryOperation(
			&Parser::parseTerm,
			&Parser::parseTerm,
			s_arithExprTokens
		);
	}

	/* Parse term */
	Parser::Result Parser::parseTerm() {

		if (m_token->matchSymbol("++") ||
		    m_token->matchSymbol("--")) {

			Position position = m_token->position;
			const std::string& operation = m_token->value;

			advance();

			NodeP call = BEA_OK(NodeP, parseCall());

			auto node = std::make_unique<UnaryOperationNode>(
				operation, std::move(call)
			);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Binary operation */
		static const TokenMatchList s_termTokens = {
			{TokenType::Symbol, "*"},
			{TokenType::Symbol, "/"},
			{TokenType::Symbol, "<<"},
			{TokenType::Symbol, ">>"},
		};
		return parseBinaryOperation(
			&Parser::parseCall,
			&Parser::parseCall,
			s_termTokens
		);
	}

	/* Parse call */
	Parser::Result Parser::parseCall() {

		NodeP factor = BEA_OK(NodeP, parseFactor());
		Node *factorP = factor.get();

		factor = BEA_OK(NodeP, parseCallExtension(std::move(factor)));
		while (factor.get() != factorP) {

			factorP = factor.get();
			factor = BEA_OK(NodeP, parseCallExtension(std::move(factor)));
		}
		return Result(std::move(factor));
	}

	/* Parse call extension */
	Parser::Result Parser::parseCallExtension(NodeP factor) {

		if (m_token->matchSymbol("("))
			return parseFunctionCall(std::move(factor));

		else if (m_token->matchSymbol("."))
			return parseNamedAccess(std::move(factor));

		else if (m_token->matchSymbol("["))
			return parseIndexedAccess(std::move(factor));

		else if (m_token->matchSymbol("?"))
			return parseCarryResult(std::move(factor));

		else if (m_token->matchSymbol("++") ||
			 m_token->matchSymbol("--"))
			return parseIncrementDecrement(std::move(factor));

		else if (m_token->matchSymbol(".."))
			return parseRange(std::move(factor));

		else if (m_token->matchKeyword("as"))
			return parseCallCast(std::move(factor));

		return std::move(factor);
	}

	/* Parse function call */
	Parser::Result Parser::parseFunctionCall(NodeP factor) {

		Position position = m_token->position;
		advance(Skip::EndLine);

		auto node = std::make_unique<CallNode>(
			std::move(factor)
		);

		if (!m_token->matchSymbol(")")) {

			node->m_argumentNodes.emplace_back(
				BEA_OK(NodeP, parseExpr())
			);
			while (m_token->matchSymbol(",")) {

				advance(Skip::EndLine);

				node->m_argumentNodes.emplace_back(
					BEA_OK(NodeP, parseExpr())
				);
			}
		}
		skipNewlines();

		if (!m_token->matchSymbol(")"))
			return Result(
				m_token->position,
				std::format(
					"Expected ')' at end of function call but got '{}'",
					m_token->value
				)
			);
		advance();

		node->m_position = position;
		return Result(std::move(node));
	}

	/* Parse named access */
	Parser::Result Parser::parseNamedAccess(NodeP factor) {

		Position position = m_token->position;
		advance();

		if (m_token->type != TokenType::Identifier)
			return Result(
				m_token->position,
				std::format(
					"Expected member name after '.' but got '{}'",
					m_token->value
				)
			);
		Name name{m_token->value};
		advance();

		auto node = std::make_unique<NamedAccessNode>(
			std::move(factor), std::move(name)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse indexed access */
	Parser::Result Parser::parseIndexedAccess(NodeP factor) {

		Position position = m_token->position;
		advance();

		NodeP indexNode = BEA_OK(NodeP, parseExpr());

		if (!m_token->matchSymbol("]"))
			return Result(
				m_token->position,
				std::format(
					"Expected ']' after index value but got '{}'",
					m_token->value
				)
			);
		advance();

		auto node = std::make_unique<IndexedAccessNode>(
			std::move(factor), std::move(indexNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse carry result */
	Parser::Result Parser::parseCarryResult(NodeP factor) {

		Position position = m_token->position;
		advance();

		auto node = std::make_unique<CarryResultNode>(
			std::move(factor)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse postfix increment or decrement */
	Parser::Result Parser::parseIncrementDecrement(NodeP factor) {

		Position position = m_token->position;
		const std::string& operation = m_token->value;

		advance();

		auto node = std::make_unique<UnaryOperationNode>(
			operation, std::move(factor), true
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse range */
	Parser::Result Parser::parseRange(NodeP factor) {

		Position position = m_token->position;
		advance();

		NodeP endNode = BEA_OK(NodeP, parseCall());

		auto node = std::make_unique<RangeNode>(
			std::move(factor), std::move(endNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse call cast */
	Parser::Result Parser::parseCallCast(NodeP factor) {

		Position position = m_token->position;
		advance();

		NodeP typeNode = BEA_OK(NodeP, parseType());

		auto node = std::make_unique<CastNode>(
			std::move(typeNode),
			std::move(factor)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse factor */
	Parser::Result Parser::parseFactor() {

		Position position = m_token->position;

		/* Integer constant */
		if (m_token->type == TokenType::Integer) {

			const std::string& value = m_token->value;
			advance();

			std::intmax_t iValue = 0;
			std::sscanf(value.c_str(), "%jd", &iValue);

			auto node = std::make_unique<IntegerNode>(iValue);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Floating point constant */
		else if (m_token->type == TokenType::Float) {

			const std::string& value = m_token->value;
			advance();

			double fValue = 0;
			std::sscanf(value.c_str(), "%lf", &fValue);

			auto node = std::make_unique<FloatNode>(fValue);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* String constant */
		else if (m_token->type == TokenType::String) {

			const std::string& value = m_token->value;
			advance();

			auto node = std::make_unique<StringNode>(value);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Character constant */
		else if (m_token->type == TokenType::Character) {

			char value = m_token->value[0];
			advance();

			auto node = std::make_unique<IntegerNode>((std::intmax_t)value);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Variable access */
		else if (m_token->type == TokenType::Identifier) {

			Name name = BEA_OK(NodeP, parseName());

			auto node = std::make_unique<VariableAccessNode>(
				std::move(name)
			);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Unary operation */
		else if (m_token->matchSymbol("+") ||
			 m_token->matchSymbol("-")) {

			const std::string& operation = m_token->value;
			advance();

			NodeP factor = BEA_OK(NodeP, parseFactor());

			auto node = std::make_unique<UnaryOperationNode>(
				operation, std::move(factor)
			);
			node->m_position = position;

			return Result(std::move(node));
		}

		/* Others */
		else if (m_token->matchKeyword("new"))
			return parseNewExpression();

		else if (m_token->matchKeyword("cast"))
			return parseCastExpression();

		else if (m_token->matchSymbol("["))
			return parseListInitializer();

		else if (m_token->matchSymbol("("))
			return parseGrouping();

		return Result(
			position,
			std::format(
				"Unexpected token '{}'",
				m_token->value
			)
		);
	}

	/* Parse "new" expression */
	Parser::Result Parser::parseNewExpression() {

		Position position = m_token->position;
		advance();

		NodeP typeNode = BEA_OK(NodeP, parseType());

		if (!m_token->matchSymbol("("))
			return Result(
				m_token->position,
				std::format(
					"Expected '(' after type but got '{}'",
					m_token->value
				)
			);
		advance(Skip::EndLine);

		/* constructor arguments */
		std::vector<NodeP> argumentNodes;
		if (!m_token->matchSymbol(")")) {

			argumentNodes.emplace_back(
				BEA_OK(NodeP, parseExpr())
			);
			while (m_token->matchSymbol(",")) {

				advance(Skip::EndLine);

				argumentNodes.emplace_back(
					BEA_OK(NodeP, parseExpr())
				);
			}
		}
		skipNewlines();

		if (!m_token->matchSymbol(")"))
			return Result(
				m_token->position,
				std::format(
					"Expected ')' after arguments but got '{}'",
					m_token->value
				)
			);
		advance();

		auto node = std::make_unique<NewNode>(std::move(typeNode));
		node->m_position = position;

		node->m_argumentNodes = std::move(argumentNodes);

		return Result(std::move(node));
	}

	/* Parse cast expression */
	Parser::Result Parser::parseCastExpression() {

		Position position = m_token->position;
		advance();

		if (!m_token->matchSymbol("<"))
			return Result(
				m_token->position,
				std::format(
					"Expected '<' after 'cast' but got '{}'",
					m_token->value
				)
			);
		advance(Skip::EndLine);

		NodeP typeNode = BEA_OK(NodeP, parseType());

		if (!m_token->matchSymbol(">"))
			return Result(
				m_token->position,
				std::format(
					"Expected '>' after cast type but got '{}'",
					m_token->value
				)
			);
		advance(Skip::EndLine);

		if (!m_token->matchSymbol("("))
			return Result(
				m_token->position,
				std::format(
					"Expected '(' after '>' but got '{}'",
					m_token->value
				)
			);
		advance(Skip::EndLine);

		NodeP valueNode = BEA_OK(NodeP, parseExpr());

		if (!m_token->matchSymbol(")"))
			return Result(
				m_token->position,
				std::format(
					"Expected ')' after cast value but got '{}'",
					m_token->value
				)
			);
		advance();

		auto node = std::make_unique<CastNode>(
			std::move(typeNode), std::move(valueNode)
		);
		node->m_position = position;

		return Result(std::move(node));
	}

	/* Parse list initializer */
	Parser::Result Parser::parseListInitializer() {

		Position position = m_token->position;
		advance(Skip::EndLine);

		std::vector<NodeP> itemNodes;

		itemNodes.emplace_back(
			BEA_OK(NodeP, parseExpr())
		);
		while (m_token->matchSymbol(",")) {

			advance(Skip::EndLine);

			itemNodes.emplace_back(
				BEA_OK(NodeP, parseExpr())
			);
		}
		skipNewlines();

		if (!m_token->matchSymbol("]"))
			return Result(
				m_token->position,
				std::format(
					"Expected ']' at end of list initializer but got '{}'",
					m_token->value
				)
			);
		advance();

		auto node = std::make_unique<ListNode>();
		node->m_position = position;

		node->m_itemNodes = std::move(itemNodes);

		return Result(std::move(node));
	}

	/* Parse grouping */
	Parser::Result Parser::parseGrouping() {

		Position position = m_token->position;
		advance(Skip::EndLine);

		NodeP node = BEA_OK(NodeP, parseExpr());
		skipNewlines();

		if (!m_token->matchSymbol(")"))
			return Result(
				m_token->position,
				std::format(
					"Expected ')' after expression but got '{}'",
					m_token->value
				)
			);
		advance();

		return std::move(node);
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

					Position valuePosition = m_token->position;
					advance(Skip::EndLine);

					auto valueNode = std::make_unique<IntegerNode>(value);
					valueNode->m_position = valuePosition;

					node->m_genericArgumentNodes.emplace_back(
						std::move(valueNode)
					);
				}
				else node->m_genericArgumentNodes.emplace_back(
						BEA_OK(NodeP, parseType())
				);
			}
			skipNewlines();

			if (m_token->matchSymbol(">>")) /* Weird case to account for */
				m_token->value = ">"; /* NOTE: This is stupid don't do this */

			else if (!m_token->matchSymbol(">")) return Result(
				m_token->position,
				std::format(
					"Expected '>' after generic arguments but got '{}'",
					m_token->value
				)
			);
			else advance(Skip::EndLine);
		}
		return Result(std::move(node));
	}

	/* Parse name */
	Parser::NameResult Parser::parseName() {

		if (m_token->type != TokenType::Identifier)
			return NameResult(
				m_token->position,
				std::format(
					"Expected name but got '{}'",
					m_token->value
				)
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

	/* Parse function parameter */
	Parser::FuncParameterResult Parser::parseFuncParameter() {

		if (m_token->type != TokenType::Identifier)
			return FuncParameterResult(
				m_token->position,
				std::format(
					"Expected parameter name but got '{}'",
					m_token->value
				)
			);
		Name name{m_token->value};
		advance();

		if (!m_token->matchSymbol(":"))
			return FuncParameterResult(
				m_token->position,
				std::format(
					"Expected ':' after parameter name but got '{}'",
					m_token->value
				)
			);
		advance();

		NodeP typeNode = BEA_OK(NodeFuncParameter, parseType());

		return FuncParameterResult(NodeFuncParameter{
			.name = std::move(name),
			.typeNode = std::move(typeNode),
			.valueNode = nullptr
		});
	}

	/* Parse generic parameter */
	Parser::GenericParameterResult Parser::parseGenericParameter() {

		if (m_token->type != TokenType::Identifier)
			return GenericParameterResult(
				m_token->position,
				std::format(
					"Expected parameter name but got '{}'",
					m_token->value
				)
			);
		Name name{m_token->value};
		advance();

		/* Generic type */
		GenericType type = GenericType::Type;
		if (m_token->matchSymbol(":")) {

			advance();

			if (m_token->value == "Type")
				type = GenericType::Type;
			else if (m_token->value == "Int")
				type = GenericType::Int;
			else
				return GenericParameterResult(
					m_token->position,
					std::format(
						"Expected parameter type but got '{}'",
						m_token->value
					)
				);
			advance();
		}

		/* Generic value */
		NodeP defaultNode = nullptr;
		if (m_token->matchSymbol("=")) {

			advance();
			if (m_token->type == TokenType::Integer) {

				std::intmax_t value = 0;
				std::sscanf(m_token->value.c_str(), "%jd", &value);

				defaultNode = std::make_unique<IntegerNode>(value);
				defaultNode->m_position = m_token->position;

				advance();
			}
			else defaultNode = BEA_OK(NodeGenericParameter, parseType());
		}

		return GenericParameterResult(NodeGenericParameter{
			.name = std::move(name),
			.type = type,
			.defaultNode = std::move(defaultNode)
		});
	}
}
