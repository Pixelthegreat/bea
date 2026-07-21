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
		for (std::unique_ptr<Node>& argument: node.m_genericArguments) {

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
			m_stream << " " << node.m_operation << ")";
		}
		else {
			m_stream << "(" << node.m_operation << " ";
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
		visit(node.getNode());

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
}
