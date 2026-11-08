#include <format>
#include <bea/core.hpp>
#include <bea/analyzer/localresolver.hpp>

namespace bea {

	/* Visit integer constant */
	Result<None> LocalResolver::visitInteger(IntegerNode& node) {

		node.m_atom.type = std::make_unique<TypeID>(
			m_context.m_intType
		);
		return Result<None>();
	}

	/* Visit float constant */
	Result<None> LocalResolver::visitFloat(FloatNode& node) {

		node.m_atom.type = std::make_unique<TypeID>(
			m_context.m_floatType
		);
		return Result<None>();
	}

	/* Visit string constant */
	Result<None> LocalResolver::visitString(StringNode& node) {

		node.m_atom.type = std::make_unique<TypeID>(
			m_context.m_stringType
		);
		return Result<None>();
	}

	/* Visit binary operation */
	Result<None> LocalResolver::visitBinaryOperation(BinaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_leftNode));
		BEA_OK(None, visit(*node.m_rightNode));

		if (!node.m_leftNode->m_atom.type->equalTo(
			*node.m_rightNode->m_atom.type
		    ))
			return Result<None>(
				node.m_position,
				std::format(
					"Mismatched types ({} and {})",
					*node.m_leftNode->m_atom.type,
					*node.m_rightNode->m_atom.type
				)
			);
		node.m_atom.type = std::make_unique<TypeID>(
			*node.m_leftNode->m_atom.type
		);

		/* Check if operator is implemented */
		Type *type = node.m_atom.type->type;
		if (!type || type->base <= BaseType::F64)
			return Result<None>();

		const char *interface = "";
		if (node.m_operation == "+")
			interface = "std::operator::Add";
		else if (node.m_operation == "-")
			interface = "std::operator::Subtract";
		else if (node.m_operation == "*")
			interface = "std::operator::Multiply";
		else if (node.m_operation == "/")
			interface = "std::operator::Divide";
		else if (node.m_operation == "%")
			interface = "std::operator::Modulo";
		else if (node.m_operation == "|")
			interface = "std::operator::BitwiseOr";
		else if (node.m_operation == "&")
			interface = "std::operator::BitwiseAnd";
		else if (node.m_operation == "^")
			interface = "std::operator::BitwiseXor";
		else if (node.m_operation == "<<")
			interface = "std::operator::LeftShift";
		else if (node.m_operation == ">>")
			interface = "std::operator::RightShift";
		else if (node.m_operation == "==")
			interface = "std::operator::Equals";
		else if (node.m_operation == "<")
			interface = "std::operator::LessThan";
		else if (node.m_operation == ">")
			interface = "std::operator::GreaterThan";
		else if (node.m_operation == "<=") /* less than equals = not greater than */
			interface = "std::operator::GreaterThan";
		else if (node.m_operation == ">=") /* greater than equals = not less than */
			interface = "std::operator::LessThan";

		if (!interface) return Result<None>();

		/* Find appropriate interface implementation */
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name == interface &&
			    implementation.type->generics.empty())
				return Result<None>();
		}
		return Result<None>(
			node.m_position,
			std::format(
				"Invalid operation (Type {} doesn't "
				"implement {} interface)",
				type->name,
				interface
			)
		);
	}

	/* Get type for type identifier */
	std::unique_ptr<TypeID> LocalResolver::registerTypeID(
		std::unique_ptr<TypeID> typeID
	) {
		typeID->type = m_context.getType(typeID->name, true).value;
		return std::move(typeID);
	}
}
