#include <bea/core.hpp>
#include <bea/optimizer/constantfolder.hpp>

namespace bea {

	/* Visit unary operation */
	Result<None> ConstantFolder::visitUnaryOperation(UnaryOperationNode& node) {

		Node& rNode = *node.m_node;
		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(rNode))
		);

		IntegerNode *intNode = dynamic_cast<IntegerNode *>(
			node.m_node.get()
		);
		FloatNode *floatNode = dynamic_cast<FloatNode *>(
			node.m_node.get()
		);
		if (!intNode && !floatNode)
			return Mode::Nothing;

		/* not (constant) -> (constant) */
		if (node.m_operation == "not") {

			std::intmax_t value = !intNode->m_value;

			auto newIntNode = std::make_unique<IntegerNode>(value);
			newIntNode->m_atom.type = std::make_unique<TypeID>(
				*node.m_node->m_atom.type
			);
			m_optimizedNodes.emplace_back(
				std::move(newIntNode)
			);
		}

		/* (- constant) -> (constant) */
		else if (node.m_operation == "-") {

			if (intNode) {

				std::intmax_t value = -intNode->m_value;

				auto newIntNode = std::make_unique<IntegerNode>(value);
				newIntNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_node->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newIntNode)
				);
			}
			else if (floatNode) {

				double value = -floatNode->m_value;

				auto newFloatNode = std::make_unique<FloatNode>(value);
				newFloatNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_node->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newFloatNode)
				);
			}
		}

		/* (~ constant) -> (constant) */
		else if (node.m_operation == "~") {

			std::intmax_t value = ~intNode->m_value;

			auto newIntNode = std::make_unique<IntegerNode>(value);
			newIntNode->m_atom.type = std::make_unique<TypeID>(
				*node.m_node->m_atom.type
			);
			m_optimizedNodes.emplace_back(
				std::move(newIntNode)
			);
		}
		return Mode::Replace;
	}

	/* Visit binary operation */
	Result<None> ConstantFolder::visitBinaryOperation(BinaryOperationNode& node) {

		Node& rLeftNode = *node.m_leftNode;
		Node& rRightNode = *node.m_rightNode;

		node.m_leftNode = getReplacement(
			std::move(node.m_leftNode),
			BEA_OK(None, visit(rLeftNode))
		);
		node.m_rightNode = getReplacement(
			std::move(node.m_rightNode),
			BEA_OK(None, visit(rRightNode))
		);

		IntegerNode *leftIntNode = dynamic_cast<IntegerNode *>(
			node.m_leftNode.get()
		);
		IntegerNode *rightIntNode = dynamic_cast<IntegerNode *>(
			node.m_rightNode.get()
		);
		FloatNode *leftFloatNode = dynamic_cast<FloatNode *>(
			node.m_leftNode.get()
		);
		FloatNode *rightFloatNode = dynamic_cast<FloatNode *>(
			node.m_rightNode.get()
		);
		if (!leftIntNode && !leftFloatNode)
			return Mode::Nothing;

		/* Operation on integers */
		if (leftIntNode) {

			std::intmax_t value = 0;

			if (node.m_operation == "+")
				value = leftIntNode->m_value + rightIntNode->m_value;
			else if (node.m_operation == "-")
				value = leftIntNode->m_value - rightIntNode->m_value;
			else if (node.m_operation == "*")
				value = leftIntNode->m_value * rightIntNode->m_value;
			else if (node.m_operation == "/")
				value = leftIntNode->m_value / rightIntNode->m_value;
			else if (node.m_operation == "%")
				value = leftIntNode->m_value % rightIntNode->m_value;
			else if (node.m_operation == "|")
				value = leftIntNode->m_value | rightIntNode->m_value;
			else if (node.m_operation == "&")
				value = leftIntNode->m_value & rightIntNode->m_value;
			else if (node.m_operation == "^")
				value = leftIntNode->m_value & rightIntNode->m_value;
			else if (node.m_operation == "<<")
				value = leftIntNode->m_value << rightIntNode->m_value;
			else if (node.m_operation == ">>")
				value = leftIntNode->m_value >> rightIntNode->m_value;
			else if (node.m_operation == "==")
				value = leftIntNode->m_value == rightIntNode->m_value;
			else if (node.m_operation == "!=")
				value = leftIntNode->m_value != rightIntNode->m_value;
			else if (node.m_operation == "<")
				value = leftIntNode->m_value < rightIntNode->m_value;
			else if (node.m_operation == ">")
				value = leftIntNode->m_value > rightIntNode->m_value;
			else if (node.m_operation == "<=")
				value = leftIntNode->m_value <= rightIntNode->m_value;
			else if (node.m_operation == ">=")
				value = leftIntNode->m_value >= rightIntNode->m_value;

			/* Create int node */
			auto intNode = std::make_unique<IntegerNode>(value);
			intNode->m_atom.type = std::make_unique<TypeID>(
				*node.m_atom.type
			);
			m_optimizedNodes.emplace_back(
				std::move(intNode)
			);
		}

		/* Operation on floats */
		else if (leftFloatNode) {

			double value = 0;
			std::intmax_t iValue = 0;

			if (node.m_operation == "+")
				value = leftFloatNode->m_value + rightFloatNode->m_value;
			else if (node.m_operation == "-")
				value = leftFloatNode->m_value - rightFloatNode->m_value;
			else if (node.m_operation == "*")
				value = leftFloatNode->m_value * rightFloatNode->m_value;
			else if (node.m_operation == "/")
				value = leftFloatNode->m_value / rightFloatNode->m_value;
			else if (node.m_operation == "%")
				value = std::fmod(
					leftFloatNode->m_value,
					rightFloatNode->m_value
				);
			else if (node.m_operation == "==")
				iValue = leftFloatNode->m_value == rightFloatNode->m_value;
			else if (node.m_operation == "!=")
				iValue = leftFloatNode->m_value != rightFloatNode->m_value;
			else if (node.m_operation == "<")
				iValue = leftFloatNode->m_value < rightFloatNode->m_value;
			else if (node.m_operation == ">")
				iValue = leftFloatNode->m_value > rightFloatNode->m_value;
			else if (node.m_operation == "<=")
				iValue = leftFloatNode->m_value <= rightFloatNode->m_value;
			else if (node.m_operation == ">=")
				iValue = leftFloatNode->m_value >= rightFloatNode->m_value;

			/* Create integer node */
			if (node.m_operation == "==" ||
			    node.m_operation == "!=" ||
			    node.m_operation == "<" ||
			    node.m_operation == ">" ||
			    node.m_operation == "<=" ||
			    node.m_operation == ">=") {

				auto intNode = std::make_unique<IntegerNode>(iValue);
				intNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(intNode)
				);
			}

			/* Create float node */
			else {
				auto floatNode = std::make_unique<FloatNode>(value);
				floatNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(floatNode)
				);
			}
		}
		return Mode::Replace;
	}

	/* Visit "cast" expression */
	Result<None> ConstantFolder::visitCast(CastNode& node) {

		Node& rTypeNode = *node.m_typeNode;
		Node& rValueNode = *node.m_valueNode;

		node.m_typeNode = getReplacement(
			std::move(node.m_typeNode),
			BEA_OK(None, visit(rTypeNode))
		);
		node.m_valueNode = getReplacement(
			std::move(node.m_valueNode),
			BEA_OK(None, visit(rValueNode))
		);
		Type *leftType = node.m_typeNode->m_atom.type->type;
		Type *rightType = node.m_valueNode->m_atom.type->type;

		IntegerNode *intNode = dynamic_cast<IntegerNode *>(
			node.m_valueNode.get()
		);
		FloatNode *floatNode = dynamic_cast<FloatNode *>(
			node.m_valueNode.get()
		);

		/* Cast integer */
		if (intNode) {

			std::intmax_t value = intNode->m_value;
			double fValue = 0.0;

			switch (rightType->base) {
				case BaseType::U8:
				case BaseType::I8:
					value &= 0xff; break;
				case BaseType::U16:
				case BaseType::I16:
					value &= 0xffff; break;
				case BaseType::U32:
				case BaseType::I32:
					value &= 0xffffffff; break;
			}
			switch (leftType->base) {
				case BaseType::U8:
				case BaseType::I8:
					value &= 0xff; break;
				case BaseType::U16:
				case BaseType::I16:
					value &= 0xffff; break;
				case BaseType::U32:
				case BaseType::I32:
					value &= 0xffffffff; break;
				case BaseType::F32:
				case BaseType::F64:
					fValue = (double)value; break;
			}

			/* Float result */
			if (leftType->base == BaseType::F32 ||
			    leftType->base == BaseType::F64) {

				auto newFloatNode =
					std::make_unique<FloatNode>(fValue);

				newFloatNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_typeNode->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newFloatNode)
				);
			}

			/* Integer result */
			else {
				auto newIntNode =
					std::make_unique<IntegerNode>(value);

				newIntNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_typeNode->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newIntNode)
				);
			}
			return Mode::Replace;
		}

		/* Cast float */
		else if (floatNode) {

			double value = floatNode->m_value;
			std::intmax_t iValue = (std::intmax_t)value;

			switch (leftType->base) {
				case BaseType::U8:
				case BaseType::I8:
					iValue &= 0xff; break;
				case BaseType::U16:
				case BaseType::I16:
					iValue &= 0xffff; break;
				case BaseType::U32:
				case BaseType::I32:
					iValue &= 0xffffffff; break;
			}

			/* Float result */
			if (leftType->base == BaseType::F32 ||
			    leftType->base == BaseType::F64) {

				auto newFloatNode =
					std::make_unique<FloatNode>(value);

				newFloatNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_typeNode->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newFloatNode)
				);
			}

			/* Integer result */
			else {
				auto newIntNode =
					std::make_unique<IntegerNode>(iValue);

				newIntNode->m_atom.type = std::make_unique<TypeID>(
					*node.m_typeNode->m_atom.type
				);
				m_optimizedNodes.emplace_back(
					std::move(newIntNode)
				);
			}
			return Mode::Replace;
		}
		return Mode::Nothing;
	}
}
