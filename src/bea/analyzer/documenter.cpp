#include <bea/core.hpp>
#include <bea/analyzer/documenter.hpp>

namespace bea {

	/* Parse comment */
	Result<None> CommentParser::parse() {

		m_position.column--;
		advanceRaw(); /* Skip slash and star or double slash */
		advanceRaw();
		advanceRaw();

		while (m_cc == '\n')
			advance();

		while (m_cc) {

			/* Parse directive */
			if (m_cc != '@') {

				while (!std::strchr("\n", m_cc))
					advance();
				advance();
				continue;
			}
			advance();

			m_sstream.str("");
			while (!std::strchr(" \t\r", m_cc)) {

				if (m_cc != '\n') m_sstream << m_cc;
				advance();
			}
			std::string command = m_sstream.str();

			while (m_cc && std::strchr(" \t\r", m_cc))
				advance();

			if (command == "brief")
				BEA_OK(None, parseDescription(false));
			else if (command == "desc")
				BEA_OK(None, parseDescription(true));
			else if (command == "param")
				BEA_OK(None, parseParameter(false));
			else if (command == "gparam")
				BEA_OK(None, parseParameter(true));
			else if (command == "returns")
				BEA_OK(None, parseReturns());
			else
				return Result<None>(
					m_position,
					std::format(
						"Unrecognized documentation "
						"directive '@{}'",
						command
					)
				);
		}
		return Result<None>();
	}

	/* Advance position */
	void CommentParser::advanceRaw() {

		if (!m_cc) return;

		if (m_stream.eof()) m_cc = 0;
		else m_cc = m_stream.get();

		m_position.column++;
		if (m_cc == '\n') {

			m_position.line++;
			m_position.column = 0;
		}
		else if (m_cc == EOF)
			m_cc = 0;
	}

	void CommentParser::advance() {

		m_pc = m_cc;
		advanceRaw();

		if (!m_cc || m_pc != '\n')
			return;
		while (m_cc && std::strchr(" \t\r", m_cc))
			advanceRaw();

		if (m_cc == '*') {

			advanceRaw();
			if (m_cc == '/') {

				m_cc = 0;
				return;
			}
		}
		while (m_cc && std::strchr(" \t\r", m_cc))
			advanceRaw();
	}

	/* Parse description (@brief, @desc) */
	Result<None> CommentParser::parseDescription(bool isLong) {

		m_sstream.str("");
		while (!std::strchr("\n", m_cc)) {

			if (m_cc == '\\') {

				advance();
				if (!m_cc) break;
			}
			if (m_cc != '\n')
				m_sstream << m_cc;
			else
				m_sstream << ' ';
			advance();
		}
		if (isLong)
			m_description = m_sstream.str();
		else
			m_briefDescription = m_sstream.str();
		return Result<None>();
	}

	/* Parse parameter (@param, @gparam) */
	Result<None> CommentParser::parseParameter(bool isGeneric) {

		m_sstream.str("");
		while (!std::strchr(" \r\t\n", m_cc)) {

			m_sstream << m_cc;
			advance();
		}
		std::string name = m_sstream.str();
		if (name.empty()) return Result<None>(
			m_position,
			std::format(
				"Expected parameter name after '@{}param'",
				isGeneric? "g": ""
			)
		);
		while (m_cc && std::strchr(" \r\t", m_cc))
			advance();

		/* Parse parameter type */
		std::string type{};
		if (!isGeneric) {

			m_sstream.str("");
			std::size_t gLevel = 0;

			while (m_cc && (m_cc != ' ' || gLevel)) {

				if (m_cc == '<') gLevel++;
				else if (m_cc == '>') gLevel--;

				if (m_cc != '\n') m_sstream << m_cc;
				advance();
			}
			type = m_sstream.str();
			if (type.empty()) return Result<None>(
				m_position,
				"Expected parameter type after name"
			);
			while (m_cc && std::strchr(" \r\t", m_cc))
				advance();
		}

		/* Parse description */
		m_sstream.str("");
		while (!std::strchr("\n", m_cc)) {

			if (m_cc == '\\') {

				advance();
				if (!m_cc) break;
			}
			if (m_cc != '\n')
				m_sstream << m_cc;
			else
				m_sstream << ' ';
			advance();
		}

		/* Create parameter */
		Parameter parameter = {
			.name = std::move(name),
			.type = std::move(type),
			.description = m_sstream.str(),
		};
		if (isGeneric)
			m_genericParameters.push_back(std::move(parameter));
		else
			m_parameters.push_back(std::move(parameter));
		return Result<None>();
	}

	/* Parse return value (@returns) */
	Result<None> CommentParser::parseReturns() {

		m_sstream.str("");
		std::size_t gLevel = 0;

		while (m_cc && (m_cc != ' ' || gLevel)) {

			if (m_cc == '<') gLevel++;
			else if (m_cc == '>') gLevel--;

			if (m_cc != '\n') m_sstream << m_cc;
			advance();
		}
		std::string type = m_sstream.str();
		if (type.empty()) return Result<None>(
			m_position,
			"Expected parameter type after name"
		);
		while (m_cc && std::strchr(" \r\t", m_cc))
			advance();

		/* Parse description */
		m_sstream.str("");
		while (!std::strchr("\n", m_cc)) {

			if (m_cc == '\\') {

				advance();
				if (!m_cc) break;
			}
			if (m_cc != '\n')
				m_sstream << m_cc;
			else
				m_sstream << ' ';
			advance();
		}

		/* Create parameter */
		Parameter parameter = {
			.type = std::move(type),
			.description = m_sstream.str(),
		};
		m_returns = std::move(parameter);
		return Result<None>();
	}

	/* Analyze node */
	Result<None> Documenter::analyze(Node& node) {

		startTable("");

		BlockNode *blockNode = dynamic_cast<BlockNode *>(&node);
		if (!blockNode) return Result<None>::fail();

		/* Write classes */
		startTable("classes");
		for (std::unique_ptr<Node>& node: blockNode->m_nodes) {

			if (!dynamic_cast<ClassStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endTable(); /* classes */

		/* Write functions */
		startTable("functions");
		for (std::unique_ptr<Node>& node: blockNode->m_nodes) {

			if (!dynamic_cast<FuncExprStatementNode *>(node.get()) &&
			    !dynamic_cast<FuncBlockStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endTable(); /* functions */

		/* Write globals */
		startTable("globals");
		for (std::unique_ptr<Node>& node: blockNode->m_nodes) {

			if (!dynamic_cast<LetStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endTable(); /* globals */

		endTable();
		return Result<None>();
	}

	/* Visit "let" statement */
	Result<None> Documenter::visitLetStatement(LetStatementNode& node) {

		if (!node.m_commentToken || !node.m_public)
			return Result<None>();

		std::istringstream istream(node.m_commentToken->value);
		CommentParser parser(
			istream, node.m_commentToken->position
		);
		BEA_OK(None, parser.parse());
		for (const Name& name: node.m_names) {

			startTable(name);

			std::string type = std::format("{}", *node.m_typeNode->m_atom.type);
			writeTableEntry("type", type);
			writeTableEntry("briefDescription", parser.m_briefDescription);
			writeTableEntry("description", parser.m_description);

			endTable();
		}
		return Result<None>();
	}

	/* Visit "func" statement */
	Result<None> Documenter::visitFuncStatement(FuncStatementNode& node) {

		if (!node.m_commentToken || !node.m_public)
			return Result<None>();

		std::istringstream istream(node.m_commentToken->value);
		CommentParser parser(
			istream, node.m_commentToken->position
		);
		BEA_OK(None, parser.parse());

		/* Create table for function */
		std::string name = std::format(
			"{}#{}", node.m_name,
			node.m_atom.overloadID
		);
		startTable(name);

		std::ostringstream sigStream;

		if (node.m_abstract) sigStream << "abstract ";
		if (node.m_static) sigStream << "static ";

		sigStream << "func " << node.m_name;

		std::size_t count = 0;
		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			if (!count++) sigStream << "<";
			else sigStream << ", ";
			sigStream << parameter.name;
		}
		if (count) sigStream << ">";

		count = 0;
		sigStream << "(";
		for (const NodeFuncParameter& parameter: node.m_parameters) {

			if (count++) sigStream << ", ";
			sigStream <<
				parameter.name << ": " <<
				*parameter.typeNode;
		}
		sigStream << ") -> " << *node.m_returnTypeNode;

		writeTableEntry(
			"signature",
			sigStream.str()
		);
		writeTableEntry(
			"briefDescription",
			parser.m_briefDescription
		);
		writeTableEntry(
			"description",
			parser.m_description
		);

		/* Write generic parameters */
		startTable("genericParameters");
		for (
			const CommentParser::Parameter& parameter:
			parser.m_genericParameters
		)
			writeTableEntry(
				parameter.name,
				parameter.description
			);
		endTable(); /* genericParameters */

		/* Write parameters */
		startTable("parameters");
		for (
			const CommentParser::Parameter& parameter:
			parser.m_parameters
		) {
			startTable(parameter.name);
			writeTableEntry("type", parameter.type);
			writeTableEntry("description", parameter.description);
			endTable();
		}
		endTable(); /* parameters */

		/* Write returns */
		startTable("returns");
		writeTableEntry("type", parser.m_returns.type);
		writeTableEntry("description", parser.m_returns.description);
		endTable(); /* returns */

		endTable();
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> Documenter::visitFuncExprStatement(FuncExprStatementNode& node) {

		return visitFuncStatement(node);
	}

	/* Visit "func" statement with block body */
	Result<None> Documenter::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		return visitFuncStatement(node);
	}

	/* Visit class field statement */
	Result<None> Documenter::visitClassFieldStatement(ClassFieldStatementNode& node) {

		if (!node.m_commentToken || !node.m_public)
			return Result<None>();

		std::istringstream istream(node.m_commentToken->value);
		CommentParser parser(
			istream, node.m_commentToken->position
		);
		BEA_OK(None, parser.parse());

		startTable(node.m_name);

		std::string type = std::format("{}", *node.m_typeNode->m_atom.type);
		writeTableEntry("type", type);
		writeTableEntry("briefDescription", parser.m_briefDescription);
		writeTableEntry("description", parser.m_description);

		endTable();
		return Result<None>();
	}

	/* Visit "impl" statement */
	Result<None> Documenter::visitImplStatement(ImplStatementNode& node) {

		std::string type = std::format("{}", *node.m_typeNode->m_atom.type);
		writeListEntry(type);

		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> Documenter::visitClassStatement(ClassStatementNode& node) {

		if (!node.m_commentToken || !node.m_public)
			return Result<None>();

		std::istringstream istream(node.m_commentToken->value);
		CommentParser parser(
			istream, node.m_commentToken->position
		);
		BEA_OK(None, parser.parse());

		/* Create table for class */
		startTable(node.m_name);

		std::ostringstream sigStream;
		if (node.m_abstract) sigStream << "abstract ";

		if (node.m_copy) sigStream << "struct ";
		else sigStream << "class ";

		sigStream << node.m_name;

		std::size_t count = 0;
		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			if (!count++) sigStream << "<";
			else sigStream << ", ";
			sigStream << parameter.name;
		}
		if (count) sigStream << ">";

		writeTableEntry(
			"signature",
			sigStream.str()
		);
		writeTableEntry(
			"briefDescription",
			parser.m_briefDescription
		);
		writeTableEntry(
			"description",
			parser.m_description
		);

		/* Write generic parameters */
		startTable("genericParameters");
		for (
			const CommentParser::Parameter& parameter:
			parser.m_genericParameters
		)
			writeTableEntry(
				parameter.name,
				parameter.description
			);
		endTable(); /* genericParameters */

		BlockNode *bodyNode = static_cast<BlockNode *>(node.m_bodyNode.get());

		/* Write fields */
		startTable("fields");
		for (std::unique_ptr<Node>& node: bodyNode->m_nodes) {

			if (!dynamic_cast<ClassFieldStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endTable(); /* fields */

		/* Write functions */
		startTable("functions");
		for (std::unique_ptr<Node>& node: bodyNode->m_nodes) {

			if (!dynamic_cast<FuncExprStatementNode *>(node.get()) &&
			    !dynamic_cast<FuncBlockStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endTable(); /* functions */

		/* Write implementations */
		startList("implementations");
		for (std::unique_ptr<Node>& node: bodyNode->m_nodes) {

			if (!dynamic_cast<ImplStatementNode *>(node.get()))
				continue;
			visit(*node);
		}
		endList(); /* implementations */

		endTable();
		return Result<None>();
	}

	/* Print tabs */
	std::ostream& Documenter::tabs() {

		for (std::size_t i = 0; i < m_tabLevel; i++)
			m_stream << '\t';
		return m_stream;
	}

	/* Start JSON table ("{ ... }") */
	void Documenter::startTable(const char *name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": {";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '{';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}
	void Documenter::startTable(const std::string& name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": {";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '{';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}
	void Documenter::startTable(const Name& name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": {";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '{';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}

	/* End JSON table */
	void Documenter::endTable() {

		m_levelCounts.pop_back();
		m_tabLevel--;

		m_stream << '\n';
		tabs() << '}';
		if (!m_tabLevel) m_stream << '\n';
	}

	/* Start JSON list ("[ ... ]") */
	void Documenter::startList(const char *name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": [";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '[';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}
	void Documenter::startList(const std::string& name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": [";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '[';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}
	void Documenter::startList(const Name& name) {

		if (m_tabLevel) {

			if (m_levelCounts[m_tabLevel-1])
				m_stream << ',';
			m_stream << '\n';
			tabs() << '"' << name << "\": [";

			m_levelCounts[m_tabLevel-1]++;
		}
		else m_stream << '[';

		m_tabLevel++;
		m_levelCounts.push_back(0);
	}

	/* End JSON list */
	void Documenter::endList() {

		m_levelCounts.pop_back();
		m_tabLevel--;

		m_stream << '\n';
		tabs() << ']';
		if (!m_tabLevel) m_stream << '\n';
	}

	/* Write entry for table */
	void Documenter::writeTableEntry(
		const std::string& name,
		const std::string& value
	) {
		if (m_levelCounts[m_tabLevel-1])
			m_stream << ',';
		m_stream << '\n';
		tabs() << '"' << name << "\": \"" << value << '"';

		m_levelCounts[m_tabLevel-1]++;
	}

	/* Write entry for list */
	void Documenter::writeListEntry(
		const std::string& value
	) {
		if (m_levelCounts[m_tabLevel-1])
			m_stream << ',';
		m_stream << '\n';
		tabs() << '"' << value << '"';

		m_levelCounts[m_tabLevel-1]++;
	}
}
