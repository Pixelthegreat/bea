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

	/* Visit type */
	Result<None> LocalResolver::visitType(TypeNode& node) {

		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(node.m_name)
		);
		TypeID& typeID = *node.m_atom.type;
		typeID.generics.resize(node.m_genericArgumentNodes.size());

		std::size_t i = 0;
		for (const std::unique_ptr<Node>& argument: node.m_genericArgumentNodes) {

			GenericInfo& generic = typeID.generics[i++];
			IntegerNode *intNode =
				dynamic_cast<IntegerNode *>(
					argument.get()
				);

			/* Generic int */
			if (intNode) {

				generic.type = GenericType::Int;
				generic.substitutedValue = intNode->m_value;
			}

			/* Generic type */
			else {
				generic.type = GenericType::Type;

				BEA_OK(None, visit(*argument));
				generic.substitutedType = std::move(
					argument->m_atom.type
				);
			}
		}
		return validateTypeID(typeID, node.m_position);
	}

	/* Visit variable access */
	Result<None> LocalResolver::visitVariableAccess(VariableAccessNode& node) {

		VariableInfo *info = BEA_OK(
			None, m_context.m_currentFrame->get(
				node.m_name, true
			)
		);
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(*info->type)
		);
		node.m_atom.overloadID = info->overloadID;

		return Result<None>();
	}

	/* Visit assignment */
	Result<None> LocalResolver::visitAssignment(AssignmentNode& node) {

		BEA_OK(None, visit(*node.m_leftNode));
		BEA_OK(None, visit(*node.m_rightNode));

		/* Check assignability */
		if (!dynamic_cast<VariableAccessNode *>(node.m_leftNode.get()) &&
		    !dynamic_cast<NamedAccessNode *>(node.m_leftNode.get()) &&
		    !dynamic_cast<IndexedAccessNode *>(node.m_leftNode.get()))
			return Result<None>(
				node.m_position,
				"Can't assign to value directly"
			);

		/* Check types */
		node.m_atom.typeEquality =
			node.m_leftNode->m_atom.type->equalTo(
				*node.m_rightNode->m_atom.type
			);
		if (!node.m_atom.typeEquality)
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

		const char *interface = nullptr;
		if (node.m_operation == "+=")
			interface = "std::operator::Add";
		else if (node.m_operation == "-=")
			interface = "std::operator::Subtract";
		else if (node.m_operation == "*=")
			interface = "std::operator::Multiply";
		else if (node.m_operation == "/=")
			interface = "std::operator::Divide";
		else if (node.m_operation == "%=")
			interface = "std::operator::Modulo";
		else if (node.m_operation == "|=")
			interface = "std::operator::BitwiseOr";
		else if (node.m_operation == "&=")
			interface = "std::operator::BitwiseAnd";
		else if (node.m_operation == "^=")
			interface = "std::operator::BitwiseXor";
		else if (node.m_operation == "<<=")
			interface = "std::operator::BitwiseLeftShift";
		else if (node.m_operation == ">>=")
			interface = "std::operator::BitwiseRightShift";

		if (!interface) return Result<None>();

		/* Find appropriate interface implementation */
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name == interface &&
			    implementation.type->generics.empty()) {

				node.m_atom.operationImpl = &implementation;
				return Result<None>();
			}
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

	/* Visit binary operation */
	Result<None> LocalResolver::visitBinaryOperation(BinaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_leftNode));
		BEA_OK(None, visit(*node.m_rightNode));

		node.m_atom.typeEquality =
			node.m_leftNode->m_atom.type->equalTo(
				*node.m_rightNode->m_atom.type
			);
		if (!node.m_atom.typeEquality)
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

		const char *interface = nullptr;
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
			interface = "std::operator::BitwiseLeftShift";
		else if (node.m_operation == ">>")
			interface = "std::operator::BitwiseRightShift";
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
			    implementation.type->generics.empty()) {

				node.m_atom.operationImpl = &implementation;
				return Result<None>();
			}
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

	/* Visit "let" statement */
	Result<None> LocalResolver::visitLetStatement(LetStatementNode& node) {

		std::vector<VariableInfo *> infos;
		for (const Name& name: node.m_names) {

			VariableInfo& info = m_context.m_currentFrame->m_values[name];
			infos.push_back(&info);
		}
		if (!node.m_typeNode && !node.m_valueNode)
			return Result<None>(
				node.m_position,
				"Variable(s) must have type or value"
			);

		/* Check type */
		if (node.m_typeNode)
			BEA_OK(None, visit(*node.m_typeNode));
		if (node.m_valueNode)
			BEA_OK(None, visit(*node.m_valueNode));

		if (node.m_typeNode)
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*node.m_typeNode->m_atom.type
				)
			);
		else {
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*node.m_valueNode->m_atom.type
				)
			);
			if (node.m_atom.type->type->base == BaseType::Function)
				return Result<None>(
					node.m_position,
					"Cannot assign function to variable"
				);
		}
		for (VariableInfo *info: infos) {

			if (info->type) continue;
			info->type = std::make_unique<TypeID>(*node.m_atom.type);
		}

		/* Check value type */
		node.m_atom.typeEquality = TypeEquality::Equivalent;
		if (node.m_typeNode && node.m_valueNode) {

			node.m_atom.typeEquality =
				node.m_atom.type->equalTo(
					*node.m_valueNode->m_atom.type
				);
			if (!node.m_atom.typeEquality)
				return Result<None>(
					node.m_position,
					std::format(
						"Mismatched types ({} and {})",
						*node.m_atom.type,
						*node.m_valueNode->m_atom.type
					)
				);
		}
		return Result<None>();
	}

	/* Visit "func" statement (common) */
	Result<None> LocalResolver::visitFuncStatement(FuncStatementNode& node) {

		BEA_OK(None, visit(*node.m_returnTypeNode));

		for (NodeGenericParameter& parameter: node.m_genericParameters) {
			if (parameter.defaultNode)
				BEA_OK(None, visit(*parameter.defaultNode));
		}
		for (NodeFuncParameter& parameter: node.m_parameters)
			BEA_OK(None, visit(*parameter.typeNode));

		/* Get associated variable */
		VariableInfo *info = BEA_OK(None, m_context.m_currentFrame->get(
			node.m_name, node.m_atom.overloadID
		));
		Type *type = info->type->type;

		Frame& frame = *m_context.pushFrame();
		// TODO: this

		/* Create temporary constants for generic parameters */
		for (const GenericInfo& generic: type->function.genericParameters) {

			if (generic.type == GenericType::Type) {

				std::unique_ptr<Type>& type =
					frame.m_types[generic.name];
				type = std::make_unique<Type>(
					BaseType::Any,
					generic.name.c_str()
				);
			}
			else if (generic.type == GenericType::Int) {

				VariableInfo& entry = frame.newEntry(generic.name);
				entry.type = std::make_unique<TypeID>(m_context.m_intType);
			}
		}

		/* Create temporary variables */
		for (
			const std::pair<Name, VariableInfo>& parameter:
			type->function.parameters
		) {
			VariableInfo& entry = frame.newEntry(parameter.first);

			entry.type = registerTypeID(std::make_unique<TypeID>(
				*parameter.second.type
			));
		}
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> LocalResolver::visitFuncExprStatement(FuncExprStatementNode& node) {

		BEA_OK(None, visitFuncStatement(node));

		BEA_OK(None, visit(*node.m_bodyNode));
		m_context.popFrame();

		return Result<None>();
	}

	/* Visit "func" statement with block body */
	Result<None> LocalResolver::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		BEA_OK(None, visitFuncStatement(node));

		m_returnType = node.m_returnTypeNode->m_atom.type.get();
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();

		return Result<None>();
	}

	/* Get type for type identifier */
	std::unique_ptr<TypeID> LocalResolver::registerTypeID(
		std::unique_ptr<TypeID> typeID
	) {
		if (!typeID->type)
			typeID->type = m_context.getType(
				typeID->name, true
			).value;
		return std::move(typeID);
	}

	/* Validate type identifier */
	Result<None> LocalResolver::validateTypeID(
		const TypeID& typeID,
		const Position& position
	) {
		if (typeID.type->base != BaseType::Object &&
		    !typeID.generics.size())
			return Result<None>();

		if (typeID.type->base != BaseType::Object)
			return Result<None>(
				position,
				"Type must be object to have "
				"generic arguments"
			);

		if (typeID.generics.size() <
		    typeID.type->object.genericParameters.size())
			return Result<None>(
				position,
				"Too few generic arguments"
			);
		else if (typeID.generics.size() >
			 typeID.type->object.genericParameters.size())
			return Result<None>(
				position,
				"Too many generic arguments"
			);

		/* Compare arguments */
		for (std::size_t i = 0; i < typeID.generics.size(); i++) {

			const GenericInfo& generic =
				typeID.generics[i];
			const GenericInfo& parameter =
				typeID.type->object.genericParameters[i];

			if (generic.type != parameter.type)
				return Result<None>(
					position,
					std::format(
						"Mismatched generic argument types "
						"({} and {})",
						generic.type == GenericType::Int?
						"Int": "Type",
						parameter.type == GenericType::Int?
						"Int": "Type"
					)
				);

			if (generic.substitutedType)
				BEA_OK(None, validateTypeID(
					*generic.substitutedType,
					position
				));
		}
		return Result<None>();
	}
}
