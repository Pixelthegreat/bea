#pragma once

#include <sstream>
#include <functional>
#include <bea/core.hpp>
#include <bea/token.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Parser
	 */
	class Parser {
	public:
		using NoneResult = bea::Result<None>;
		using Result = bea::Result<std::unique_ptr<Node>>;
		using NameResult = bea::Result<Name>;
		using FuncParameterResult = bea::Result<NodeFuncParameter>;
		using GenericParameterResult = bea::Result<NodeGenericParameter>;

		using TokenMatchList = std::vector<std::tuple<TokenType, const char *>>;

		Parser(const std::vector<std::unique_ptr<Token>>& tokens)
			: m_tokens(tokens) {}

		/*
		 * Parse file
		 */
		Result parse();
	private:
		const std::vector<std::unique_ptr<Token>>& m_tokens;
		std::size_t m_tokenIndex = 0;

		const Token *m_token = nullptr;

		std::istringstream m_sstream;

		/*
		 * Advance to next token
		 */
		enum class Skip {
			None = 0,
			Comment,
			EndLine,

			Count,
		};
		void advanceRaw();
		void advance(Skip skipLevel = Skip::Comment);

		/*
		 * Helpers
		 */
		void skipNewlines();

		bool matchTokenList(
			const TokenMatchList& pairs,
			const Token *token
		);

		Result parseBinaryOperation(
			std::function<Result(Parser& parser)> leftFunc,
			std::function<Result(Parser& parser)> rightFunc,
			const TokenMatchList& pairs,
			std::size_t iterations = -1
		);

		/*
		 * Grammar rules
		 */
		Result parseModuleStatement();

		Result parseFileStatement();
		Result parseUseStatement();
		Result parseClassStatement();
		Result parseFuncStatement();
		Result parseLetStatement();
		Result parsePubStatement();

		Result parseClassBlock();
		Result parseClassBlockStatement();
		Result parseClassPubStatement();
		Result parseClassImplStatement();
		Result parseClassImplBlock();
		Result parseClassField();

		NoneResult parseBlock(BlockNode& block);
		Result parseStatement();
		Result parseIfStatement();
		Result parseWhichStatement();
		Result parseWhichBlock();
		Result parseWhichBlockStatement();
		Result parseForStatement();
		Result parseWhileStatement();
		Result parseReturnStatement();
		Result parseContinueStatement();
		Result parseBreakStatement();

		Result parseExpr();
		Result parseAssignExpr();
		Result parseCompExpr();
		Result parseArithExpr();
		Result parseTerm();
		Result parseCall();

		Result parseCallExtension(std::unique_ptr<Node> factor);
		Result parseFunctionCall(std::unique_ptr<Node> factor);
		Result parseNamedAccess(std::unique_ptr<Node> factor);
		Result parseIndexedAccess(std::unique_ptr<Node> factor);
		Result parseCarryResult(std::unique_ptr<Node> factor);
		Result parseIncrementDecrement(std::unique_ptr<Node> factor);
		Result parseRange(std::unique_ptr<Node> factor);

		Result parseFactor();
		Result parseNewExpression();
		Result parseCastExpression();
		Result parseListInitializer();
		Result parseGrouping();

		Result parseType();

		NameResult parseName();
		FuncParameterResult parseFuncParameter();
		GenericParameterResult parseGenericParameter();
	};
}
