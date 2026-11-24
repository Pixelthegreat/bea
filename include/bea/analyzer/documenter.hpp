/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/container.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * Parser for documentation comment
	 */
	class CommentParser {
	public:
		/*
		 * Parsed information
		 */
		struct Parameter {
			std::string name;
			std::string type;
			std::string description;
		};

		/*
		 * Comment parser
		 */
		CommentParser(
			std::istream& stream,
			const Position& position
		)
			: m_stream(stream),
			  m_position(position) {}

		std::string m_briefDescription;
		std::string m_description;
		std::vector<Parameter> m_parameters;
		std::vector<Parameter> m_genericParameters;
		Parameter m_returns;

		/*
		 * Parse comment
		 */
		Result<None> parse();
	private:
		std::istream& m_stream;
		Position m_position;
		std::ostringstream m_sstream;
		char m_cc = 1, m_pc = 0;

		/*
		 * Advance position
		 */
		void advanceRaw();
		void advance();

		/*
		 * Parse description (@brief, @desc)
		 */
		Result<None> parseDescription(bool isLong = false);

		/*
		 * Parse parameter (@param, @gparam)
		 */
		Result<None> parseParameter(bool isGeneric = false);

		/*
		 * Parse return value (@returns)
		 */
		Result<None> parseReturns();
	};

	/*
	 * Documentation generator phase
	 */
	class Documenter : public NodeAnalyzerVisitor {
	public:
		Documenter(
			Context& context,
			std::ostream& stream
		)
			: NodeAnalyzerVisitor(context),
			  m_stream(stream) {}

		virtual Result<None> analyze(Node& node) override;

		virtual Result<None> visitLetStatement(LetStatementNode& node) override;
		Result<None> visitFuncStatement(FuncStatementNode& node);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
		virtual Result<None> visitClassFieldStatement(ClassFieldStatementNode& node) override;
		virtual Result<None> visitImplStatement(ImplStatementNode& node) override;
		virtual Result<None> visitClassStatement(ClassStatementNode& node) override;
	private:
		std::ostream& m_stream;
		std::size_t m_tabLevel = 0;
		std::vector<std::size_t> m_levelCounts{};

		/*
		 * JSON stuff
		 */
		std::ostream& tabs();

		void startTable(const char *name);
		void startTable(const std::string& name);
		void startTable(const Name& name);
		void endTable();

		void startList(const char *name);
		void startList(const std::string& name);
		void startList(const Name& name);
		void endList();

		void writeTableEntry(
			const std::string& name,
			const std::string& value
		);
		void writeListEntry(
			const std::string& value
		);
	};
}
