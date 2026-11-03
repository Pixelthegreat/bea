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
			m_stream << " " << node.m_operation << ")";
		}
		else {
			m_stream << "(" << node.m_operation << " ";
			visit(node.getNode());
			m_stream << ")";
		}
		return Result<None>();
	}

	/* Print assignment */
	Result<None> NodePrinter::visitAssignment(AssignmentNode& node) {

		m_stream << "(";
		visit(node.getLeftNode());
		m_stream << " " << node.m_operation << " ";
		visit(node.getRightNode());
		m_stream << ")";

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

	/* Print list initializer */
	Result<None> NodePrinter::visitList(ListNode& node) {

		m_stream << "[";

		std::size_t i = 0;
		for (const std::unique_ptr<Node>& item: node.m_itemNodes) {

			if (i++) m_stream << ", ";
			visit(*item);
		}
		m_stream << "]";

		return Result<None>();
	}

	/* Print function call */
	Result<None> NodePrinter::visitCall(CallNode& node) {

		visit(node.getNode());

		m_stream << "(";

		std::size_t i = 0;
		for (const std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			if (i++) m_stream << ", ";
			visit(*argument);
		}
		m_stream << ")";

		return Result<None>();
	}

	/* Print named member access */
	Result<None> NodePrinter::visitNamedAccess(NamedAccessNode& node) {

		visit(node.getNode());
		m_stream << "." << node.getName();

		return Result<None>();
	}

	/* Print indexed member access */
	Result<None> NodePrinter::visitIndexedAccess(IndexedAccessNode& node) {

		visit(node.getNode());
		m_stream << "[";
		visit(node.getIndexNode());
		m_stream << "]";

		return Result<None>();
	}

	/* Print range */
	Result<None> NodePrinter::visitRange(RangeNode& node) {

		visit(node.getStartNode());
		m_stream << "..";
		visit(node.getEndNode());

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
		Node& elseClauseNode = node.getElseClauseNode();
		if (&elseClauseNode) {

			m_stream << " else ";
			visit(elseClauseNode);
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

	/* Print "return" statement */
	Result<None> NodePrinter::visitReturnStatement(ReturnStatementNode& node) {

		m_stream << "return";

		Node& value = node.getNode();
		if (&value) {

			m_stream << " ";
			visit(value);
		}
		return Result<None>();
	}

	/* Print "continue" statement */
	Result<None> NodePrinter::visitContinueStatement(ContinueStatementNode& node) {

		m_stream << "continue";
		return Result<None>();
	}

	/* Print "break" statement */
	Result<None> NodePrinter::visitBreakStatement(BreakStatementNode& node) {

		m_stream << "break";
		return Result<None>();
	}

	/* Print "module" statement */
	Result<None> NodePrinter::visitModuleStatement(ModuleStatementNode& node) {

		m_stream << "module " << node.getName();
		return Result<None>();
	}

	/* Print "pub" statement */
	Result<None> NodePrinter::visitPubStatement(PubStatementNode& node) {

		m_stream << "pub ";
		visit(node.getNode());

		return Result<None>();
	}

	/* Print "use" statement */
	Result<None> NodePrinter::visitUseStatement(UseStatementNode& node) {

		m_stream << "use " << node.getName();

		const Name& alias = node.getAlias();
		if (!alias.empty()) m_stream << " as " << alias;

		return Result<None>();
	}

	/* Print "let" statement */
	Result<None> NodePrinter::visitLetStatement(LetStatementNode& node) {

		m_stream << "let ";

		std::size_t i = 0;
		for (const Name& name: node.getNames()) {

			if (i++) m_stream << ", ";
			m_stream << name;
		}
		Node& type = node.getTypeNode();
		if (&type) {

			if (i > 1) m_stream << " ";
			m_stream << ": ";
			visit(type);
		}
		Node& value = node.getValueNode();
		if (&value) {

			m_stream << " = ";
			visit(value);
		}
		return Result<None>();
	}

	/* Print "func" statement with expression for body */
	Result<None> NodePrinter::visitFuncExprStatement(FuncExprStatementNode& node) {

		visitFuncStatementCommon(static_cast<FuncStatementNode&>(node));

		Node& returnType = node.getReturnTypeNode();
		if (&returnType) {

			m_stream << " -> ";
			visit(returnType);
		}
		m_stream << " => ";
		visit(node.getBodyNode());

		return Result<None>();
	}

	/* Print "func" statement with block for body */
	Result<None> NodePrinter::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		visitFuncStatementCommon(static_cast<FuncStatementNode&>(node));

		Node& returnType = node.getReturnTypeNode();
		if (&returnType) {

			m_stream << " -> ";
			visit(returnType);
		}
		m_stream << " ";
		visit(node.getBodyNode());

		return Result<None>();
	}

	/* Visit common aspects of "func" statement */
	void NodePrinter::visitFuncStatementCommon(FuncStatementNode& node) {

		if (node.m_abstract)
			m_stream << "abstract ";
		if (node.m_static)
			m_stream << "static ";

		m_stream << "func " << node.getName();

		std::size_t i = 0;
		for (NodeGenericParameter& parameter: node.m_genericParameters) {

			if (!i++) m_stream << "<";
			else m_stream << ", ";

			m_stream << parameter.name << ": " << parameter.type;
			if (parameter.defaultNode) {

				m_stream << " = ";
				visit(*parameter.defaultNode);
			}
		}
		if (i) m_stream << ">";

		i = 0;
		m_stream << "(";
		for (NodeFuncParameter& parameter: node.m_parameters) {

			if (i++) m_stream << ", ";

			m_stream << parameter.name << ": ";
			visit(*parameter.typeNode);

			if (parameter.valueNode) {

				m_stream << " = ";
				visit(*parameter.valueNode);
			}
		}
		m_stream << ")";

		if (node.m_useDefault)
			m_stream << "!";
	}

	/* Visit class field statement */
	Result<None> NodePrinter::visitClassFieldStatement(ClassFieldStatementNode& node) {

		m_stream << node.getName();

		Node& typeNode = node.getTypeNode();
		if (&typeNode) {

			m_stream << ": ";
			visit(typeNode);
		}
		Node& valueNode = node.getValueNode();
		if (&valueNode) {

			m_stream << " = ";
			visit(valueNode);
		}
		return Result<None>();
	}

	/* Visit "impl" statement */
	Result<None> NodePrinter::visitImplStatement(ImplStatementNode& node) {

		m_stream << "impl ";
		visit(node.getTypeNode());
		m_stream << " ";
		visit(node.getBodyNode());

		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> NodePrinter::visitClassStatement(ClassStatementNode& node) {

		if (node.m_abstract)
			m_stream << "abstract ";

		if (node.m_copy)
			m_stream << "struct ";
		else
			m_stream << "class ";
		m_stream << node.getName();

		std::size_t i = 0;
		for (NodeGenericParameter& parameter: node.m_genericParameters) {

			if (!i++) m_stream << "<";
			else m_stream << ", ";

			m_stream << parameter.name << ": " << parameter.type;
			if (parameter.defaultNode) {

				m_stream << " = ";
				visit(*parameter.defaultNode);
			}
		}
		if (i) m_stream << ">";
		m_stream << " ";

		visit(node.getBodyNode());

		return Result<None>();
	}
}
