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

	/* Visit unary operation */
	Result<None> LocalResolver::visitUnaryOperation(UnaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_node));

		if (node.m_operation == "++" || node.m_operation == "--") {

			if (!dynamic_cast<VariableAccessNode *>(node.m_node.get()) &&
			    !dynamic_cast<NamedAccessNode *>(node.m_node.get()) &&
			    !dynamic_cast<IndexedAccessNode *>(node.m_node.get()))
				return Result<None>(
					node.m_position,
					"Can't increment or decrement "
					"value directly"
				);
		}

		/* Pick type off of operation */
		if (node.m_operation == "not")
			node.m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
		else
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*node.m_node->m_atom.type
				)
			);

		/* Check if operator is implemented */
		Type *type = node.m_atom.type->type;
		if (!type || type->base <= BaseType::F64)
			return Result<None>();

		const char *interface = nullptr;
		if (node.m_operation == "-")
			interface = "std::operator::Multiply";
		else if (node.m_operation == "~")
			interface = "std::operator::BitwiseNot";
		else if (node.m_operation == "++")
			interface = "std::operator::Add";
		else if (node.m_operation == "--")
			interface = "std::operator::Subtract";
		else if (node.m_operation == "not")
			interface = "std::operator::True";

		if (!interface) return Result<None>();

		if (type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		/* Find appropriate interface implementation */
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name != interface ||
			    !implementation.type->generics.empty())
				continue;

			node.m_atom.operationImpl = &implementation;
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

		if (type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

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

		if (node.m_operation == "or" ||
		    node.m_operation == "and")
			node.m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
		else
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
		else if (node.m_operation == "or" ||
			 node.m_operation == "and")
			interface = "std::operator::True";

		if (!interface) return Result<None>();

		if (type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

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

	/* Visit "new" expression */
	Result<None> LocalResolver::visitNew(NewNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));

		for (std::unique_ptr<Node>& argument: node.m_argumentNodes)
			BEA_OK(None, visit(*argument));

		Type *type = node.m_atom.type->type;

		/* Check type for constructor */
		if (type->base != BaseType::Object ||
		    type->object.flagAbstract)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't construct type {}",
					*node.m_atom.type
				)
			);

		VariableInfo *variable = nullptr;
		for (std::pair<Name, VariableInfo>& pair: type->object.publicMembers) {

			Type *fType = pair.second.type->type;

			if (pair.first != "init" || !fType ||
			    fType->base != BaseType::Function ||
			    fType->function.parameters.size() !=
			    node.m_argumentNodes.size())
				continue;

			/* Check arguments */
			BEA_OK(None, validateCall(
				fType,
				node.m_argumentNodes,
				type->object.genericParameters,
				node.m_atom.type->generics,
				node.m_atom.argumentTypeEqualities
			));
			TypeEquality lowest = TypeEquality::Equivalent;
			for (TypeEquality equality: node.m_atom.argumentTypeEqualities) {

				if (equality < lowest)
					lowest = equality;
			}
			if (!!lowest) {

				variable = &pair.second;
				break;
			}
		}

		/* Error */
		if (!variable) {

			std::ostringstream stream;
			stream <<
				"No matching constructor for " <<
				*node.m_atom.type << "(";

			std::size_t i = 0;
			for (const std::unique_ptr<Node>& argument: node.m_argumentNodes) {

				if (i++) stream << ", ";
				stream << *argument->m_atom.type;
			}
			stream << ")";

			return Result<None>(
				node.m_position,
				stream.str()
			);
		}
		return Result<None>();
	}

	/* Visit "cast" expression */
	Result<None> LocalResolver::visitCast(CastNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		BEA_OK(None, visit(*node.m_valueNode));

		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(
				*node.m_typeNode->m_atom.type
			)
		);
		node.m_atom.typeEquality = node.m_atom.type->equalTo(
			*node.m_valueNode->m_atom.type
		);
		if (!!node.m_atom.typeEquality)
			return Result<None>();

		/* "Any" container */
		Type *type = node.m_atom.type->type;
		Type *other = node.m_valueNode->m_atom.type->type;

		if (type->base == BaseType::Any) {

			node.m_atom.flagDynamic = true;
			return Result<None>();
		}
		else if (type->base != BaseType::Object ||
			 other->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't cast type {} to {}",
					*node.m_valueNode->m_atom.type,
					*node.m_atom.type
				)
			);

		/* Look for class implementation */
		node.m_atom.flagDynamic = true;
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			node.m_atom.typeEquality =
				node.m_valueNode->m_atom.type->equalTo(
					*implementation.type
				);
			if (node.m_atom.typeEquality != TypeEquality::Equivalent)
				continue;

			node.m_atom.operationImpl = &implementation;
			break;
		}
		return Result<None>();
	}

	/* Visit named member access */
	Result<None> LocalResolver::visitNamedAccess(NamedAccessNode& node) {

		BEA_OK(None, visit(*node.m_node));

		Type *type = node.m_node->m_atom.type->type;
		if (!type || type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				std::format(
					"Type {} must be object to "
					"access indexed member",
					*node.m_node->m_atom.type
				)
			);

		/* Get field */
		/* TODO: private members */
		VariableInfo *variable = nullptr;
		for (std::pair<Name, VariableInfo>& pair: type->object.publicMembers) {

			if (pair.first == node.m_name) {

				variable = &pair.second;
				break;
			}
		}
		if (!variable)
			return Result<None>(
				node.m_position,
				std::format(
					"Undefined member '{}' of type {}",
					node.m_name,
					*node.m_node->m_atom.type
				)
			);
		else if (variable->type->type &&
			 variable->type->type->base == BaseType::Function)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't directly accessed bound "
					"member '{}' of type {}",
					node.m_name,
					*node.m_node->m_atom.type
				)
			);

		/* Get substituted generic type */
		TypeID *typeID = variable->type.get();

		std::size_t i = 0;
		for (GenericInfo& parameter: type->object.genericParameters) {

			i++;

			if (parameter.type != GenericType::Type ||
			    variable->type->name.components.size() > 1 ||
			    variable->type->name.components[0] != parameter.name)
				continue;

			typeID = node.m_node->m_atom.type->
				generics[i-1].substitutedType.get();
			break;
		}
		node.m_atom.type = std::make_unique<TypeID>(*typeID);
		return Result<None>();
	}

	/* Visit indexed member access */
	Result<None> LocalResolver::visitIndexedAccess(IndexedAccessNode& node) {

		BEA_OK(None, visit(*node.m_node));
		BEA_OK(None, visit(*node.m_indexNode));

		Type *type = node.m_node->m_atom.type->type;
		if (!type || type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				std::format(
					"Type {} must be object to "
					"access indexed member",
					*node.m_node->m_atom.type
				)
			);

		/* Get implementation */
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name != "std::operator::Index")
				continue;

			node.m_atom.operationImpl = &implementation;
			break;
		}
		if (!node.m_atom.operationImpl)
			return Result<None>(
				node.m_position,
				std::format(
					"Invalid operation (Type {} doesn't "
					"implement std::operator::Index interface)",
					*node.m_node->m_atom.type
				)
			);
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(
				*node.m_atom.operationImpl->
				type->generics[0].substitutedType
			)
		);
		return Result<None>();
	}

	/* Visit "return" statement */
	Result<None> LocalResolver::visitReturnStatement(ReturnStatementNode& node) {

		BEA_OK(None, visit(*node.m_node));

		node.m_atom.typeEquality = m_returnType->equalTo(
			*node.m_node->m_atom.type
		);
		if (!node.m_atom.typeEquality)
			return Result<None>(
				node.m_position,
				std::format(
					"Mismatched return types ({} and {})",
					*m_returnType,
					*node.m_node->m_atom.type
				)
			);
		return Result<None>();
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

	/* TODO: Visit "class" statement */
	Result<None> LocalResolver::visitClassStatement(ClassStatementNode& node) {

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

	/* Validate call */
	Result<None> LocalResolver::validateCall(
		Type *type,
		std::vector<std::unique_ptr<Node>>& arguments,
		std::vector<GenericInfo>& genericParameters,
		std::vector<GenericInfo>& genericArguments,
		std::vector<TypeEquality>& typeEqualities
	) {
		typeEqualities.resize(arguments.size());

		std::size_t i = 0;
		for (std::unique_ptr<Node>& argument: arguments) {

			/* Get generic type */
			const TypeID *compareType =
				(type->function.parameters.data() + i)->
				second.type.get();

			std::size_t j = 0;
			for (const GenericInfo& parameter: genericParameters) {

				j++;
				if (parameter.type != GenericType::Type)
					continue;

				if (compareType->name.components.size() > 1 ||
				    parameter.name != compareType->name.components[0])
					continue;

				compareType = genericArguments[j-1].substitutedType.get();
				break;
			}

			/* Compare types */
			typeEqualities[i++] = compareType->equalTo(
				*argument->m_atom.type
			);
		}
		return Result<None>();
	}
}
