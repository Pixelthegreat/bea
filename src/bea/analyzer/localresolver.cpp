/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <format>
#include <bea/core.hpp>
#include <bea/analyzer/localresolver.hpp>

namespace bea {

	/* Visit block */
	Result<None> LocalResolver::visitBlock(BlockNode& node) {

		m_blockLevel++;

		bool noPushBlockFrame = m_noPushBlockFrame;
		m_noPushBlockFrame = false;

		if (m_blockLevel == 1)
			noPushBlockFrame = true;

		if (!noPushBlockFrame) m_context.pushFrame();

		for (std::size_t i = 0; i < node.m_nodes.size(); i++)
			BEA_OK(None, visit(*node.m_nodes[i]));

		if (!noPushBlockFrame) m_context.popFrame();
		m_blockLevel--;
		return Result<None>();
	}

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

		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>("std::string::String")
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
		node.m_atom.scopeID = m_context.m_frames.size()-1;

		return Result<None>();
	}

	/* Visit unary operation */
	Result<None> LocalResolver::visitUnaryOperation(UnaryOperationNode& node) {

		BEA_OK(None, visit(*node.m_node));

		if (node.m_operation == "++" || node.m_operation == "--") {

			if (!dynamic_cast<VariableAccessNode *>(node.m_node.get()) &&
			    !dynamic_cast<NamedAccessNode *>(node.m_node.get()))
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
		Type *type = node.m_node->m_atom.type->type;
		if (!type) return Result<None>();

		if ((type->base == BaseType::F32 ||
		     type->base == BaseType::F64) &&
		    (node.m_operation == "not" ||
		     node.m_operation == "~" ||
		     node.m_operation == "++" ||
		     node.m_operation == "--"))
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);
		if (type->base <= BaseType::F64)
			return Result<None>();

		const char *interface = nullptr;
		if (node.m_operation == "-")
			interface = "std::operator::Subtract";
		else if (node.m_operation == "~")
			interface = "std::operator::BitwiseXor";
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
			ObjectImplementation& implementation:
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
				*node.m_node->m_atom.type,
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

		/* Get value type */
		IndexedAccessNode *indexNode =
			dynamic_cast<IndexedAccessNode *>(
				node.m_leftNode.get()
			);
		if (indexNode)
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*indexNode->m_atom.type->
					generics[0].substitutedType
				)
			);
		else
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*node.m_leftNode->m_atom.type
				)
			);

		/* Check if operator is implemented */
		Type *type = node.m_atom.type->type;

		if ((type->base == BaseType::F32 ||
		     type->base == BaseType::F64) &&
		    (node.m_operation == "|=" ||
		     node.m_operation == "&=" ||
		     node.m_operation == "^=" ||
		     node.m_operation == "<<=" ||
		     node.m_operation == ">>="))
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		else if (type->base <= BaseType::F64)
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
			ObjectImplementation& implementation:
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
				*node.m_atom.type,
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
		if (!node.m_atom.typeEquality &&
		    node.m_operation != "or" &&
		    node.m_operation != "and")
			return Result<None>(
				node.m_position,
				std::format(
					"Mismatched types ({} and {})",
					*node.m_leftNode->m_atom.type,
					*node.m_rightNode->m_atom.type
				)
			);

		/* And / or */
		if (node.m_operation == "or" ||
		    node.m_operation == "and") {

			node.m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);

			Type *leftType = node.m_leftNode->m_atom.type->type;
			Type *rightType = node.m_rightNode->m_atom.type->type;

			/* Check left */
			if (!leftType || leftType->base < BaseType::F32)
				goto checkRightType;

			else if (leftType->base != BaseType::Object)
				return Result<None>(
					node.m_leftNode->m_position,
					"Value must be integral or implement "
					"std::operator::True"
				);
			for (
				ObjectImplementation& implementation:
				leftType->object.implementations
			) {
				if (implementation.type->name != "std::operator::True" ||
				    implementation.type->generics.size())
					continue;

				node.m_atom.operationImpl = &implementation;
				goto checkRightType;
			}
			return Result<None>(
				node.m_leftNode->m_position,
				std::format(
					"Type {} doesn't implement std::operator::True",
					*node.m_leftNode->m_atom.type
				)
			);
checkRightType:
			/* Check right */
			if (!rightType || rightType->base < BaseType::F32)
				return Result<None>();

			else if (rightType->base != BaseType::Object)
				return Result<None>(
					node.m_rightNode->m_position,
					"Value must be integral or implement "
					"std::operator::True"
				);
			for (
				ObjectImplementation& implementation:
				rightType->object.implementations
			) {
				if (implementation.type->name != "std::operator::True" ||
				    implementation.type->generics.size())
					continue;

				node.m_atom.operationImpl2 = &implementation;
				return Result<None>();
			}
			return Result<None>(
				node.m_rightNode->m_position,
				std::format(
					"Type {} doesn't implement std::operator::True",
					*node.m_rightNode->m_atom.type
				)
			);
		}

		/* Comparison operations */
		if (node.m_operation == "==" ||
		    node.m_operation == "!=" ||
		    node.m_operation == "<" ||
		    node.m_operation == ">" ||
		    node.m_operation == "<=" ||
		    node.m_operation == ">=")
			node.m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
		else
			node.m_atom.type = registerTypeID(
				std::make_unique<TypeID>(
					*node.m_leftNode->m_atom.type
				)
			);

		/* Check if operator is implemented */
		Type *type = node.m_atom.type->type;
		if ((type->base == BaseType::F32 ||
		     type->base == BaseType::F64) &&
		    (node.m_operation == "|" ||
		     node.m_operation == "&" ||
		     node.m_operation == "^" ||
		     node.m_operation == "<<" ||
		     node.m_operation == ">>"))
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		if (type->base <= BaseType::F64)
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

		if (type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		/* Find appropriate interface implementation */
		for (
			ObjectImplementation& implementation:
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
				*node.m_atom.type,
				interface
			)
		);
	}

	/* Visit carry result ("?" operator) */
	Result<None> LocalResolver::visitCarryResult(CarryResultNode& node) {

		BEA_OK(None, visit(*node.m_node));

		if (m_returnType->name != "std::result::Result")
			return Result<None>(
				node.m_position,
				"Return type must be std::result::Result<T>"
			);

		TypeID& typeID = *node.m_node->m_atom.type;
		if (typeID.name != "std::result::Result")
			return Result<None>(
				node.m_position,
				"Type must be std::result::Result<T>"
			);
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(
				*node.m_node->m_atom.type->
				generics[0].substitutedType
			)
		);
		return validateTypeID(*node.m_atom.type, node.m_position);
	}

	/* Visit "new" expression */
	Result<None> LocalResolver::visitNew(NewNode& node) {

		BEA_OK(None, visit(*node.m_typeNode));
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(*node.m_typeNode->m_atom.type)
		);

		for (std::unique_ptr<Node>& argument: node.m_argumentNodes)
			BEA_OK(None, visit(*argument));

		Type *type = node.m_atom.type->type;

		/* Check type for constructor */
		if (type->base != BaseType::Object)
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
		node.m_atom.variable = variable;

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
			ObjectImplementation& implementation:
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

	/* Visit list initializer */
	Result<None> LocalResolver::visitList(ListNode& node) {

		for (std::unique_ptr<Node>& item: node.m_itemNodes)
			BEA_OK(None, visit(*item));

		/* Check types */
		TypeID& typeID = *node.m_itemNodes[0]->m_atom.type;

		node.m_atom.argumentTypeEqualities.resize(
			node.m_itemNodes.size()
		);
		node.m_atom.argumentTypeEqualities[0] =
			TypeEquality::Equivalent;

		for (std::unique_ptr<Node>& item: node.m_itemNodes) {

			node.m_atom.argumentTypeEqualities[0] =
				typeID.equalTo(*item->m_atom.type);
			if (!node.m_atom.argumentTypeEqualities[0])
				return Result<None>(
					item->m_position,
					std::format(
						"Mismatched list item types ({} and {})",
						typeID,
						*item->m_atom.type
					)
				);
		}

		/* Set type identifier */
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>("std::list::List")
		);
		GenericInfo generic{};

		generic.type = GenericType::Type;
		generic.substitutedType = registerTypeID(
			std::make_unique<TypeID>(typeID)
		);
		node.m_atom.type->generics.emplace_back(
			std::move(generic)
		);
		return Result<None>();
	}

	/* Visit function call */
	Result<None> LocalResolver::visitCall(CallNode& node) {

		for (std::unique_ptr<Node>& argument: node.m_argumentNodes)
			BEA_OK(None, visit(*argument));

		const Name *name = nullptr;
		TypeID *typeID = nullptr;
		std::unique_ptr<TypeID> pTypeID = nullptr;

		OrderedMap<Name, VariableInfo> *variables[3] = {
			nullptr, nullptr, nullptr
		};

		VariableAccessNode *varNode =
			dynamic_cast<VariableAccessNode *>(
				node.m_node.get()
			);
		NamedAccessNode *memberNode =
			dynamic_cast<NamedAccessNode *>(
				node.m_node.get()
			);

		/* Variable access */
		if (varNode) {

			name = &varNode->m_name;
			variables[0] = &m_context.m_baseFrame->m_values;

			pTypeID = std::make_unique<TypeID>(m_context.m_intType);
			typeID = pTypeID.get();
		}

		/* Member access */
		else if (memberNode) {

			name = &memberNode->m_name;
			BEA_OK(None, visit(*memberNode->m_node));

			Type *type = memberNode->m_node->m_atom.type->type;
			if (!type || type->base != BaseType::Object)
				return Result<None>(
					memberNode->m_position,
					std::format(
						"Type {} must be object to "
						"access named member",
						*memberNode->m_node->m_atom.type
					)
				);
			variables[0] = &type->object.publicMembers;
			if (m_classType == type)
				variables[1] = &type->object.privateMembers;

			typeID = memberNode->m_node->m_atom.type.get();
		}
		Type *type = typeID->type;

		/* Search variables */
		VariableInfo *variable = nullptr;
		for (auto **it = variables; *it && !variable; it++) {
			for (std::pair<Name, VariableInfo>& pair: **it) {

				Type *fType = pair.second.type->type;

				if (pair.first != *name || !fType ||
				    fType->base != BaseType::Function ||
				    fType->function.parameters.size() !=
				    node.m_argumentNodes.size())
					continue;

				/* Check arguments */
				BEA_OK(None, validateCall(
					fType,
					node.m_argumentNodes,
					type->object.genericParameters,
					typeID->generics,
					node.m_atom.argumentTypeEqualities
				));
				TypeEquality lowest = TypeEquality::Equivalent;
				for (
					TypeEquality equality:
					node.m_atom.argumentTypeEqualities
				) {
					if (equality < lowest)
						lowest = equality;
				}
				if (!!lowest) {

					variable = &pair.second;
					break;
				}
			}
		}

		/* Error */
		if (!variable) {

			std::ostringstream stream;
			stream << "No matching function for ";

			if (memberNode) stream << *typeID << "::";
			stream << *name << "(";

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
		node.m_atom.type = substituteTypes(
			*variable->type->type->function.returnType,
			type->object.genericParameters,
			typeID->generics
		);
		node.m_atom.variable = variable;
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
					"access named member",
					*node.m_node->m_atom.type
				)
			);

		/* Get field */
		VariableInfo *variable = nullptr;
		for (
			std::pair<Name, VariableInfo>& pair:
			type->object.publicMembers
		) {
			if (pair.first == node.m_name) {

				variable = &pair.second;
				break;
			}
		}
		if (!variable && m_classType == type) {

			for (
				std::pair<Name, VariableInfo>& pair:
				type->object.privateMembers
			) {
				if (pair.first == node.m_name) {

					variable = &pair.second;
					break;
				}
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

		/* Get generic substituted type */
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
		node.m_atom.variable = variable;

		node.m_atom.type = substituteTypes(
			*typeID,
			type->object.genericParameters,
			node.m_node->m_atom.type->generics,
			&node.m_atom.flagSubstituted
		);
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
			ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name != "std::operator::Index" ||
			    implementation.type->generics.size() != 2 ||
			    !implementation.type->generics[0].substitutedType ||
			    !implementation.type->generics[1].substitutedType)
				continue;

			std::unique_ptr<TypeID> compareTypeID = substituteTypes(
				*implementation.type->generics[0].substitutedType,
				type->object.genericParameters,
				node.m_node->m_atom.type->generics
			);
			if (compareTypeID->equalTo(*node.m_indexNode->m_atom.type) !=
			    TypeEquality::Equivalent)
				continue;

			node.m_atom.operationImpl = &implementation;
			break;
		}
		if (!node.m_atom.operationImpl)
			return Result<None>(
				node.m_position,
				std::format(
					"Invalid operation (Type {} doesn't "
					"implement std::operator::Index<{}> interface)",
					*node.m_node->m_atom.type,
					*node.m_indexNode->m_atom.type
				)
			);
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(
				*node.m_atom.operationImpl->
				type->generics[1].substitutedType
			)
		);
		return Result<None>();
	}

	/* Visit "if" statement */
	Result<None> LocalResolver::visitIfStatement(IfStatementNode& node) {

		for (Pair<std::unique_ptr<Node>>& pair: node.m_conditionalClauseNodes) {

			BEA_OK(None, visit(*pair[0]));
			BEA_OK(None, visit(*pair[1]));

			/* Check condition type */
			Type *type = pair[0]->m_atom.type->type;
			if (type->base < BaseType::F32)
				continue;
			else if (type->base != BaseType::Object)
				return Result<None>(
					pair[0]->m_position,
					"Invalid operation"
				);

			/* Find interface */
			bool found = false;
			for (
				const ObjectImplementation& implementation:
				type->object.implementations
			) {
				if (implementation.type->name != "std::operator::True" ||
				    !implementation.type->generics.empty())
					continue;
				found = true;
				break;
			}
			if (found) continue;

			return Result<None>(
				pair[0]->m_position,
				std::format(
					"Invalid operation (Type {} doesn't "
					"implement std::operator::True interface)",
					*pair[0]->m_atom.type
				)
			);
		}
		if (node.m_elseClauseNode)
			BEA_OK(None, visit(*node.m_elseClauseNode));
		return Result<None>();
	}

	/* Visit "which" statement */
	Result<None> LocalResolver::visitWhichStatement(WhichStatementNode& node) {

		return Result<None>("Unimplemented");
	}

	/* Visit "for" statement */
	Result<None> LocalResolver::visitForStatement(ForStatementNode& node) {

		BEA_OK(None, visit(*node.m_iterableNode));
		Frame& frame = *m_context.pushFrame();

		Type *type = node.m_iterableNode->m_atom.type->type;
		if (!type || type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		/* Look for iterable implementation */
		TypeID *typeID = nullptr;
		for (
			ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name != "std::iterator::Iterable" ||
			    implementation.type->generics.size() != 1 ||
			    !implementation.type->generics[0].substitutedType)
				continue;

			typeID = implementation.type->generics[0].substitutedType.get();
			node.m_atom.operationImpl = &implementation;
			break;
		}
		if (!typeID) return Result<None>(
			node.m_position,
			std::format(
				"Invalid operation (Type {} doesn't "
				"implement std::iterator::Iterable<T> interface)",
				*node.m_iterableNode->m_atom.type
			)
		);
		VariableInfo& itemVariable = frame.m_values[node.m_name];
		itemVariable.type = registerTypeID(
			std::make_unique<TypeID>(*typeID)
		);
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(*itemVariable.type)
		);
		node.m_atom.scopeID = m_context.m_frames.size()-1;

		/* Visit body */
		m_noPushBlockFrame = true;
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "while" statement */
	Result<None> LocalResolver::visitWhileStatement(WhileStatementNode& node) {

		m_loopLevel++;

		BEA_OK(None, visit(*node.m_conditionNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_loopLevel--;

		Type *type = node.m_conditionNode->m_atom.type->type;
		if (type->base < BaseType::F32)
			return Result<None>();
		else if (type->base != BaseType::Object)
			return Result<None>(
				node.m_position,
				"Invalid operation"
			);

		/* Find implementation */
		for (
			const ObjectImplementation& implementation:
			type->object.implementations
		) {
			if (implementation.type->name != "std::operator::True" ||
			    !implementation.type->generics.empty())
				continue;
			return Result<None>();
		}
		return Result<None>(
			node.m_position,
			std::format(
				"Invalid operation (Type {} doesn't "
				"implement std::operator::True interface)",
				*node.m_conditionNode->m_atom.type
			)
		);
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
		node.m_atom.type = registerTypeID(
			std::make_unique<TypeID>(
				*m_returnType
			)
		);
		return Result<None>();
	}

	/* Visit "continue" statement */
	Result<None> LocalResolver::visitContinueStatement(ContinueStatementNode& node) {

		if (!m_loopLevel)
			return Result<None>(
				node.m_position,
				"Attempt to continue out of loop"
			);
		return Result<None>();
	}

	/* Visit "break" statement */
	Result<None> LocalResolver::visitBreakStatement(BreakStatementNode& node) {

		if (!m_loopLevel)
			return Result<None>(
				node.m_position,
				"Attempt to break out of loop"
			);
		return Result<None>();
	}

	/* Visit "let" statement */
	Result<None> LocalResolver::visitLetStatement(LetStatementNode& node) {

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

		/* Setup variables */
		std::vector<VariableInfo *> infos;
		for (const Name& name: node.m_names) {

			VariableInfo& info = m_context.m_currentFrame->m_values[name];
			infos.push_back(&info);

			info.type = registerTypeID(
				std::make_unique<TypeID>(*node.m_atom.type)
			);
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
						"Mismatched assignment types ({} and {})",
						*node.m_atom.type,
						*node.m_valueNode->m_atom.type
					)
				);
		}
		node.m_atom.scopeID = m_context.m_frames.size()-1;
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
		node.m_atom.type = std::make_unique<TypeID>(type);

		Frame& frame = *m_context.pushFrame();
		if (!node.m_static && m_classType) {

			VariableInfo& thisVariable = frame["this"];
			thisVariable.type = std::make_unique<TypeID>(m_classType);

			for (
				const GenericInfo& parameter:
				m_classType->object.genericParameters
			) {
				GenericInfo generic{};
				generic.type = parameter.type;

				if (parameter.type == GenericType::Type)
					generic.substitutedType = registerTypeID(
						std::make_unique<TypeID>(
							parameter.name
						)
					);
				else
					generic.substitutedValue = 0;

				thisVariable.type->generics.emplace_back(
					std::move(generic)
				);
			}
		}

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
		m_noPushBlockFrame = true;
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> LocalResolver::visitFuncExprStatement(FuncExprStatementNode& node) {

		BEA_OK(None, visitFuncStatement(node));

		m_returnType = node.m_returnTypeNode->m_atom.type.get();
		BEA_OK(None, visit(*node.m_bodyNode));

		node.m_atom.typeEquality = m_returnType->equalTo(
			*node.m_bodyNode->m_atom.type
		);
		if (!node.m_atom.typeEquality)
			return Result<None>(
				node.m_position,
				std::format(
					"Mismatched return types ({} and {})",
					*m_returnType,
					*node.m_bodyNode->m_atom.type
				)
			);

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

	/* Visit "impl" statement */
	Result<None> LocalResolver::visitImplStatement(ImplStatementNode& node) {

		Type *type = node.m_typeNode->m_atom.type->type;

		if (!type ||
		    type->base != BaseType::Object ||
		    !type->object.flagAbstract)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't implement type {}",
					*node.m_typeNode->m_atom.type
				)
			);

		/* Visit body */
		Frame& frame = *m_context.pushFrame();
		frame.m_values = std::move(m_implementation->members);

		m_noPushBlockFrame = true;
		BEA_OK(None, visit(*node.m_bodyNode));

		m_implementation->members = std::move(frame.m_values);
		m_context.popFrame();

		/* Check function implementations */
		for (
			std::pair<Name, VariableInfo>& pair:
			m_implementation->members
		) {
			OrderedMap<Name, VariableInfo> *members[2] = {
				&type->object.publicMembers,
				&type->object.privateMembers,
			};
			VariableInfo *variable = nullptr;
			Type *fType = pair.second.type->type;

			for (std::size_t i = 0; i < 2; i++) {

				if (variable) break;

				/* Try and find function */
				OrderedMap<Name, VariableInfo> *pMembers = members[i];
				for (std::pair<Name, VariableInfo>& compare: *pMembers) {

					Type *compareFType = compare.second.type->type;

					if (pair.first != compare.first || !compareFType ||
					    compareFType->base != BaseType::Function ||
					    !compare.second.flagAbstract ||
					    compareFType->function.parameters.size() !=
					    fType->function.parameters.size())
						continue;
					if (compare.second.flagImplemented)
						return Result<None>(
							node.m_position,
							std::format(
								"Can't implement abstract "
								"function {}::{} twice",
								*compare.second.type,
								pair.first
							)
						);

					/* Compare return types */
					std::unique_ptr<TypeID> compareReturnType =
						substituteTypes(
							*compareFType->function.returnType,
							type->object.genericParameters,
							m_implementation->type->generics
						);

					TypeEquality typeEquality =
						compareReturnType->equalTo(
							*fType->function.returnType
						);
					if (typeEquality != TypeEquality::Equivalent)
						continue;

					/* Compare parameter types */
					TypeEquality lowest = TypeEquality::Equivalent;
					for (
						std::size_t i = 0;
						i < fType->function.parameters.size();
						i++
					) {
						std::pair<Name, VariableInfo>& fParameter =
							*(fType->function.
							  parameters.data() + i);
						std::pair<Name, VariableInfo>& cParameter =
							*(compareFType->function.
							  parameters.data() + i);

						std::unique_ptr<TypeID> compareArgumentType =
							substituteTypes(
								*cParameter.
								second.type,
								type->object.
								genericParameters,
								m_implementation->
								type->generics
							);
						typeEquality =
							compareArgumentType->equalTo(
								*fParameter.second.type
							);
						if (typeEquality < lowest)
							lowest = typeEquality;
					}
					if (lowest != TypeEquality::Equivalent)
						continue;

					pair.second.overloadID =
						compare.second.overloadID;
					variable = &compare.second;
					compare.second.flagImplemented = true;
				}
			}

			/* Error */
			if (!variable) {

				std::ostringstream stream;
				stream <<
					"No matching abstract function for " <<
					*node.m_typeNode->m_atom.type << "::" <<
					pair.first << "(";

				std::size_t i = 0;
				for (
					const std::pair<Name, VariableInfo>& parameter:
					fType->function.parameters
				) {
					if (i++) stream << ", ";
					stream << *parameter.second.type;
				}
				stream << ") -> " << *fType->function.returnType;

				return Result<None>(
					node.m_position,
					stream.str()
				);
			}
		}

		/* Determine if all necessary functions are implemented */
		for (std::pair<Name, VariableInfo>& pair: type->object.publicMembers) {

			Type *fType = pair.second.type->type;
			if (!fType || fType->base != BaseType::Function ||
			    !pair.second.flagAbstract ||
			    pair.second.flagUseDefault)
				continue;
			if (!pair.second.flagImplemented)
				return Result<None>(
					node.m_position,
					std::format(
						"Abstract function {}::{} "
						"not implemented",
						*node.m_typeNode->m_atom.type,
						pair.first
					)
				);
			pair.second.flagImplemented = false;
		}
		node.m_atom.operationImpl = m_implementation;
		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> LocalResolver::visitClassStatement(ClassStatementNode& node) {

		Type *type = BEA_OK(None, m_context.getType(node.m_name));

		Frame& frame = *m_context.pushFrame();

		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			if (parameter.defaultNode)
				BEA_OK(None, visit(*parameter.defaultNode));
		}

		/* Copy members to frame */
		for (
			std::pair<Name, VariableInfo>& pair:
			type->object.publicMembers
		) {
			VariableInfo& info = frame.m_values.newEntry(pair.first);

			info.flagConstant = pair.second.flagConstant;
			info.flagUseDefault = pair.second.flagUseDefault;
			info.flagAbstract = pair.second.flagAbstract;
			info.flagPublic = pair.second.flagPublic;
			info.flagStatic = pair.second.flagStatic;
			info.overloadID = pair.second.overloadID;
			info.scopeID = pair.second.scopeID;
			info.type = registerTypeID(
				std::make_unique<TypeID>(*pair.second.type)
			);
		}
		for (
			std::pair<Name, VariableInfo>& pair:
			type->object.privateMembers
		) {
			VariableInfo& info = frame.m_values.newEntry(pair.first);

			info.flagConstant = pair.second.flagConstant;
			info.flagUseDefault = pair.second.flagUseDefault;
			info.flagAbstract = pair.second.flagAbstract;
			info.flagPublic = pair.second.flagPublic;
			info.flagStatic = pair.second.flagStatic;
			info.overloadID = pair.second.overloadID;
			info.scopeID = pair.second.scopeID;
			info.type = registerTypeID(
				std::make_unique<TypeID>(*pair.second.type)
			);
		}

		/* Create temporary constants for generic parameters */
		/*for (const GenericInfo& parameter: type->object.genericParameters) {

			if (parameter.type == GenericType::Type) {

				std::unique_ptr<Type>& type =
					frame.m_types[parameter.name];
				type = std::make_unique<Type>(
					BaseType::Any,
					parameter.name
				);
				frame.registerType(
					parameter.name, std::move(type)
				);
			}
			else if (parameter.type == GenericType::Int) {

				VariableInfo& entry = frame.newEntry(parameter.name);
				entry.type = std::make_unique<TypeID>(m_context.m_intType);
			}
		}*/
		frame.m_types = std::move(type->object.internalTypes);

		/* Visit nodes */
		m_classType = type;

		BlockNode *block = static_cast<BlockNode *>(node.m_bodyNode.get());
		m_blockLevel++;

		for (std::size_t i = 0; i < block->m_nodes.size(); i++) {

			ImplStatementNode *implNode =
				dynamic_cast<ImplStatementNode *>(
					block->m_nodes[i].get()
				);
			if (implNode) {

				BEA_OK(None, visit(*implNode->m_typeNode));

				/* Get implementation */
				for (
					ObjectImplementation& implementation:
					type->object.implementations
				) {
					TypeEquality equality =
						implementation.type->equalTo(
							*implNode->m_typeNode->m_atom.type
						);
					if (equality == TypeEquality::Equivalent) {

						m_implementation = &implementation;
						break;
					}
				}
			}
			BEA_OK(None, visit(*block->m_nodes[i]));
		}

		m_classType = nullptr;
		m_blockLevel--;

		type->object.internalTypes = std::move(frame.m_types);
		m_context.popFrame();

		return Result<None>();
	}

	/* Get type for type identifier */
	std::unique_ptr<TypeID> LocalResolver::registerTypeID(
		std::unique_ptr<TypeID> typeID
	) {
		if (!typeID->type) {

			Result<Type *> result = m_context.getType(
				typeID->name, true
			);
			if (result.okay)
				typeID->type = result.value;
		}
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

			/* Compare types */
			TypeID& parameterType =
				*(type->function.parameters.data() + i)->second.type;

			std::unique_ptr<TypeID> compareType =
				substituteTypes(
					parameterType,
					genericParameters,
					genericArguments
				);

			typeEqualities[i++] = compareType->equalTo(
				*argument->m_atom.type
			);
		}
		return Result<None>();
	}
}
