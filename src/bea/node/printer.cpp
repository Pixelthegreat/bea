#include <bea/core.hpp>
#include <bea/node/printer.hpp>

namespace bea {

	/* Print block of statements */
	Result<None> NodePrinter::visitBlock(BlockNode& node) {

		m_stream << "{\n";
		m_tabLevel++;

		for (std::unique_ptr<Node>& node: node.m_nodes) {

			for (std::size_t i = 0; i < m_tabLevel; i++)
				m_stream << '\t';
			visit(*node);
			m_stream << std::endl;
		}
		m_tabLevel--;
		for (std::size_t i = 0; i < m_tabLevel; i++)
			m_stream << '\t';
		m_stream << '}';

		return Result<None>();
	}

	/* Print integer */
	Result<None> NodePrinter::visitInteger(IntegerNode& node) {

		m_stream << node.m_value;
		return Result<None>();
	}

	/* Print float */
	Result<None> NodePrinter::visitFloat(FloatNode& node) {

		m_stream << node.m_value;
		return Result<None>();
	}

	/* Print string */
	Result<None> NodePrinter::visitString(StringNode& node) {

		m_stream << '"' << node.m_value << '"';
		return Result<None>();
	}

	/* Print type */
	Result<None> NodePrinter::visitType(TypeNode& node) {

		m_stream << node.getName();

		std::size_t i = 0;
		for (std::unique_ptr<Node>& argument: node.m_genericArgumentNodes) {

			if (not i++) m_stream << "<";
			else m_stream << ", ";

			visit(*argument);
		}
		if (i) m_stream << ">";

		return Result<None>();
	}

	/* Print variable access */
	Result<None> NodePrinter::visitVariableAccess(VariableAccessNode& node) {

		m_stream << node.getName();
		return Result<None>();
	}

	/* Print unary operation */
	Result<None> NodePrinter::visitUnaryOperation(UnaryOperationNode& node) {

		if (node.m_postfix) {

			m_stream << "(";
			visit(node.getNode());
			m_stream << node.m_operation << ")";
		}
		else {
			m_stream << "(" << node.m_operation;
			visit(node.getNode());
			m_stream << ")";
		}
		return Result<None>();
	}

	/* Print binary operation */
	Result<None> NodePrinter::visitBinaryOperation(BinaryOperationNode& node) {

		m_stream << "(";
		visit(node.getLeftNode());
		m_stream << " " << node.m_operation << " ";
		visit(node.getRightNode());
		m_stream << ")";

		return Result<None>();
	}

	/* Print carry result */
	Result<None> NodePrinter::visitCarryResult(CarryResultNode& node) {

		visit(node.getNode());
		m_stream << "?";

		return Result<None>();
	}

	/* Print "new" expression */
	Result<None> NodePrinter::visitNew(NewNode& node) {

		m_stream << "new ";
		visit(node.getTypeNode());

		m_stream << "(";
		std::size_t i = 0;
		for (std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			if (i++) m_stream << ", ";
			visit(*argument);
		}
		m_stream << ")";

		return Result<None>();
	}

	/* Print "cast" expression */
	Result<None> NodePrinter::visitCast(CastNode& node) {

		m_stream << "cast<";
		visit(node.getTypeNode());
		m_stream << ">(";
		visit(node.getValueNode());
		m_stream << ")";

		return Result<None>();
	}

	/* Print "if" statement */
	Result<None> NodePrinter::visitIfStatement(IfStatementNode& node) {

		const char *keyword = "if";

		std::size_t i = 0;
		for (const Pair<std::unique_ptr<Node>>& clause: node.m_conditionalClauseNodes) {

			if (i++) keyword = " elif";

			m_stream << keyword << " ";
			visit(*clause[0]);
			m_stream << " ";
			visit(*clause[1]);
		}
		if (node.m_elseClauseNode) {

			m_stream << " else ";
			visit(*node.m_elseClauseNode);
		}

		return Result<None>();
	}

	/* Print normal "case" statement */
	Result<None> NodePrinter::visitCaseStatement(CaseStatementNode& node) {

		m_stream << "case ";
		visit(node.getCaseNode());

		Node& bodyNode = node.getBodyNode();
		if (!isInstance<BlockNode>(bodyNode)) /* -> statement */
			m_stream << " -> ";
		else m_stream << " ";
		visit(bodyNode);

		return Result<None>();
	}

	/* Print "case let" statement */
	Result<None> NodePrinter::visitCaseLetStatement(CaseLetStatementNode& node) {

		m_stream << "case let " << node.getName() << " = ";
		visit(node.getCaseNode());

		Node& bodyNode = node.getBodyNode();
		if (!isInstance<BlockNode>(bodyNode))
			m_stream << " -> ";
		else m_stream << " ";
		visit(bodyNode);

		return Result<None>();
	}

	/* Print "which" statement */
	Result<None> NodePrinter::visitWhichStatement(WhichStatementNode& node) {

		m_stream << "which ";
		visit(node.getValueNode());
		m_stream << " ";
		visit(node.getBodyNode());

		return Result<None>();
	}

	/* Print "for" statement */
	Result<None> NodePrinter::visitForStatement(ForStatementNode& node) {

		m_stream << "for " << node.getName() << " in ";
		visit(node.getIterableNode());
		m_stream << " ";
		visit(node.getBodyNode());

		return Result<None>();
	}

	/* Print "while" statement */
	Result<None> NodePrinter::visitWhileStatement(WhileStatementNode& node) {

		m_stream << "while ";
		visit(node.getConditionNode());
		m_stream << " ";
		visit(node.getBodyNode());

		return Result<None>();
	}
}
