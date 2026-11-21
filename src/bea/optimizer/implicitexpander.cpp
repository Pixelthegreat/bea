#include <bea/core.hpp>
#include <bea/optimizer/implicitexpander.hpp>

namespace bea {

	/* Visit node */
	Result<None> ImplicitExpander::visit(Node& node) {

		if (!&node || node.m_atom.flagExpanded)
			return Result<None>();

		Result<None> result = node.visit(*this);
		node.m_atom.flagExpanded = true;

		return result;
	}

	/* Visit unary operation */
	Result<None> ImplicitExpander::visitUnaryOperation(UnaryOperationNode& node) {

		Node& rNode = *node.m_node;
		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(rNode))
		);
		TypeID& typeID = *node.m_node->m_atom.type;
		Type *type = node.m_node->m_atom.type->type;

		if (type->base <= BaseType::F64)
			return Mode::Nothing;

		/* cast<std::operator::*>(node) */
		if (!node.m_atom.operationImpl) {

			m_optimizedNodes.emplace_back(std::move(node.m_node));
			return Mode::Replace;
		}

		auto castTypeNode = std::make_unique<TypeNode>(
			node.m_atom.operationImpl->type->name
		);
		castTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		auto castNode = std::make_unique<CastNode>(
			std::move(castTypeNode),
			std::move(node.m_node)
		);
		castNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);

		/* not cast.isTrue() */
		if (node.m_operation == "not") {

			VariableInfo *variable =
				&node.m_atom.operationImpl->type->type->
				object.publicMembers.data()[0].second;

			auto memberNode = std::make_unique<NamedAccessNode>(
				std::move(castNode), "isTrue"
			);
			auto callNode = std::make_unique<CallNode>(
				std::move(memberNode)
			);
			callNode->m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
			callNode->m_atom.variable = variable;

			auto notNode = std::make_unique<UnaryOperationNode>(
				node.m_operation,
				std::move(callNode)
			);
			notNode->m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
			m_optimizedNodes.emplace_back(std::move(notNode));
		}

		/* cast.negate() */
		else if (node.m_operation == "-") {

			VariableInfo *variable =
				&node.m_atom.operationImpl->type->type->
				object.publicMembers.data()[1].second;

			auto memberNode = std::make_unique<NamedAccessNode>(
				std::move(castNode), "negate"
			);
			auto callNode = std::make_unique<CallNode>(
				std::move(memberNode)
			);
			callNode->m_atom.type = std::make_unique<TypeID>(
				typeID
			);
			callNode->m_atom.variable = variable;

			m_optimizedNodes.emplace_back(std::move(callNode));
		}

		/* cast.bitwiseNot() */
		else if (node.m_operation == "~") {

			VariableInfo *variable =
				&node.m_atom.operationImpl->type->type->
				object.publicMembers.data()[1].second;

			auto memberNode = std::make_unique<NamedAccessNode>(
				std::move(castNode), "bitwiseNot"
			);
			auto callNode = std::make_unique<CallNode>(
				std::move(memberNode)
			);
			callNode->m_atom.type = std::make_unique<TypeID>(
				typeID
			);
			callNode->m_atom.variable = variable;

			m_optimizedNodes.emplace_back(std::move(callNode));
		}

		/* Increment or decrement */
		else if (node.m_operation == "++" ||
			 node.m_operation == "--") {

			const char *name;
			std::size_t index;

			if (node.m_operation == "++") {

				index = 1;
				name = "preIncrement";

				if (node.m_postfix) {
					index++;
					name = "postIncrement";
				}
			}
			else {
				index = 3;
				name = "preDecrement";

				if (node.m_postfix) {
					index++;
					name = "postDecrement";
				}
			}
			VariableInfo *variable =
				&node.m_atom.operationImpl->type->type->
				object.publicMembers.data()[index].second;

			/* cast.func() */
			auto memberNode = std::make_unique<NamedAccessNode>(
				std::move(castNode), name
			);
			auto callNode = std::make_unique<CallNode>(
				std::move(memberNode)
			);
			callNode->m_atom.type = std::make_unique<TypeID>(
				typeID
			);
			callNode->m_atom.variable = variable;

			m_optimizedNodes.emplace_back(std::move(callNode));
		}
		return Mode::Replace;
	}

	/* Visit assignment */
	Result<None> ImplicitExpander::visitAssignment(AssignmentNode& node) {

		Node& rLeftNode = *node.m_leftNode;
		Node& rRightNode = *node.m_rightNode;

		IndexedAccessNode *indexNode =
			dynamic_cast<IndexedAccessNode *>(
				node.m_leftNode.get()
			);
		ObjectImplementation *operationImpl = nullptr;
		if (indexNode) operationImpl =
			indexNode->m_atom.operationImpl;

		node.m_leftNode = getReplacement(
			std::move(node.m_leftNode),
			BEA_OK(None, visit(rLeftNode))
		);
		node.m_rightNode = BEA_OK(None, makeExplicitCast(
			*node.m_atom.type,
			getReplacement(
				std::move(node.m_rightNode),
				BEA_OK(None, visit(rRightNode))
			)
		));
		Type *type = node.m_atom.type->type;

		/* left op= right -> left = left op right */
		if (node.m_operation != "=") {

			static std::string s_assignOperations[] = {
				[0] = "=",
				[1] = "+",
				[2] = "-",
				[3] = "*",
				[4] = "/",
				[5] = "%",
				[6] = "|",
				[7] = "&",
				[8] = "^",
				[9] = "<<",
				[10] = ">>",
			};
			std::unique_ptr<Node> leftNode = node.m_leftNode->copy();

			const std::string *operation = nullptr;
			switch (node.m_operation[0]) {
				case '+': operation = &s_assignOperations[1]; break;
				case '-': operation = &s_assignOperations[2]; break;
				case '*': operation = &s_assignOperations[3]; break;
				case '/': operation = &s_assignOperations[4]; break;
				case '%': operation = &s_assignOperations[5]; break;
				case '|': operation = &s_assignOperations[6]; break;
				case '&': operation = &s_assignOperations[7]; break;
				case '^': operation = &s_assignOperations[8]; break;
				case '<': operation = &s_assignOperations[9]; break;
				case '>': operation = &s_assignOperations[10]; break;
			}

			/* TODO: Find a better way to do this */
			*(std::string *)&node.m_operation = s_assignOperations[0];

			/* Create binary operation */
			auto binopNode = std::make_unique<BinaryOperationNode>(
				*operation,
				std::move(leftNode),
				std::move(node.m_rightNode)
			);
			binopNode->m_atom.type = std::make_unique<TypeID>(
				*node.m_atom.type
			);
			binopNode->m_atom.operationImpl =
				node.m_atom.operationImpl;

			Node& rBinopNode = *binopNode;
			node.m_rightNode = getReplacement(
				std::move(binopNode),
				BEA_OK(None, visit(rBinopNode))
			);
		}

		/* cast<std::operator::Index<K, V>>(node).set(index, value) */
		if (!indexNode) return Mode::Nothing;

		CallNode *callNode =
			static_cast<CallNode *>(
				node.m_leftNode.get()
			);
		NamedAccessNode *memberNode =
			static_cast<NamedAccessNode *>(
				callNode->m_node.get()
			);
		memberNode->m_name = "set";
		callNode->m_atom.variable =
			&operationImpl->type->type->
			object.publicMembers.data()[1].second;

		callNode->m_argumentNodes.emplace_back(
			std::move(node.m_rightNode)
		);

		callNode->m_atom.type = std::move(
			callNode->m_atom.type->
			generics[0].substitutedType
		);
		m_optimizedNodes.emplace_back(std::move(callNode));
		return Mode::Replace;
	}

	/* Visit binary operation */
	Result<None> ImplicitExpander::visitBinaryOperation(BinaryOperationNode& node) {

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

		/* And / or */
		if (node.m_operation == "or" ||
		    node.m_operation == "and") {

			Type *leftType = node.m_leftNode->m_atom.type->type;
			Type *rightType = node.m_rightNode->m_atom.type->type;

			/* Cast left to correct type */
			if (leftType->base != BaseType::Object &&
			    leftType->base != BaseType::U8) {

				auto castTypeNode = std::make_unique<TypeNode>("Boolean");
				castTypeNode->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				node.m_leftNode = std::make_unique<CastNode>(
					std::move(castTypeNode),
					std::move(node.m_leftNode)
				);
				node.m_leftNode->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				goto checkRightType;
			}
			else if (leftType->base < BaseType::F32)
				goto checkRightType;

			node.m_leftNode = makeTrueCast(std::move(node.m_leftNode));
checkRightType:
			/* Cast right to correct type */
			if (rightType->base != BaseType::Object &&
			    rightType->base != BaseType::U8) {

				auto castTypeNode = std::make_unique<TypeNode>("Boolean");
				castTypeNode->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				node.m_rightNode = std::make_unique<CastNode>(
					std::move(castTypeNode),
					std::move(node.m_rightNode)
				);
				node.m_rightNode->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				return Mode::Nothing;
			}
			else if (rightType->base != BaseType::Object)
				return Mode::Nothing;

			node.m_rightNode = makeTrueCast(std::move(node.m_rightNode));
			return Mode::Nothing;
		}

		/* Regular operations */
		node.m_rightNode = BEA_OK(None, makeExplicitCast(
			*node.m_leftNode->m_atom.type,
			std::move(node.m_rightNode)
		));
		if (!node.m_atom.operationImpl)
			return Mode::Nothing;

		/* cast<std::operator::*>(left) */
		auto castLeftTypeNode = std::make_unique<TypeNode>(
			node.m_atom.operationImpl->type->name
		);
		castLeftTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		TypeNode& pCastLeftTypeNode = *castLeftTypeNode;

		node.m_leftNode = std::make_unique<CastNode>(
			std::move(castLeftTypeNode),
			std::move(node.m_leftNode)
		);
		node.m_leftNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);

		/* cast<std::operator::*>(right) */
		auto castRightTypeNode = std::make_unique<TypeNode>(
			node.m_atom.operationImpl->type->name
		);
		castRightTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		node.m_rightNode = std::make_unique<CastNode>(
			std::move(castRightTypeNode),
			std::move(node.m_rightNode)
		);
		node.m_rightNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);

		/* left.add(right) */
		std::pair<Name, VariableInfo>& pair =
			node.m_atom.operationImpl->type->type->
			object.publicMembers.data()[0];

		auto memberNode = std::make_unique<NamedAccessNode>(
			std::move(node.m_leftNode), pair.first
		);
		auto callNode = std::make_unique<CallNode>(
			std::move(memberNode)
		);
		callNode->m_argumentNodes.emplace_back(
			std::move(node.m_rightNode)
		);
		callNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		callNode->m_atom.variable = &pair.second;

		/* cast<T>(call) */
		auto finalCastTypeNode = std::make_unique<TypeNode>(
			node.m_atom.type->name
		);
		finalCastTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		auto finalCastNode = std::make_unique<CastNode>(
			std::move(finalCastTypeNode),
			std::move(callNode)
		);
		finalCastNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		finalCastNode->m_atom.flagDynamic = true;

		m_optimizedNodes.emplace_back(std::move(finalCastNode));
		return Mode::Replace;
	}

	/* Visit "new" expression */
	Result<None> ImplicitExpander::visitNew(NewNode& node) {

		Type *fType = node.m_atom.variable->type->type;
		for (std::size_t i = 0; i < fType->function.parameters.size(); i++) {

			std::unique_ptr<Node>& uArgument = node.m_argumentNodes[i];
			Node& rArgument = *uArgument;

			uArgument = BEA_OK(None, makeExplicitCast(
				*fType->function.parameters.
				data()[i].second.type,
				getReplacement(
					std::move(uArgument),
					BEA_OK(None, visit(rArgument))
				)
			));
		}
		return Mode::Nothing;
	}

	/* Visit list initializer */
	Result<None> ImplicitExpander::visitList(ListNode& node) {

		Node& rFirst = *node.m_itemNodes[0];
		for (std::unique_ptr<Node>& item: node.m_itemNodes) {

			Node& rItem = *item;
			item = BEA_OK(None, makeExplicitCast(
				*rFirst.m_atom.type,
				getReplacement(
					std::move(item),
					BEA_OK(None, visit(rItem))
				)
			));
		}
		return Mode::Nothing;
	}

	/* Visit function call */
	Result<None> ImplicitExpander::visitCall(CallNode& node) {

		Type *fType = node.m_atom.variable->type->type;
		for (std::size_t i = 0; i < fType->function.parameters.size(); i++) {

			std::unique_ptr<Node>& uArgument = node.m_argumentNodes[i];
			Node& rArgument = *uArgument;

			uArgument = BEA_OK(None, makeExplicitCast(
				*fType->function.parameters.
				data()[i].second.type,
				getReplacement(
					std::move(uArgument),
					BEA_OK(None, visit(rArgument))
				)
			));
		}
		return Mode::Nothing;
	}

	/* Visit indexed access */
	Result<None> ImplicitExpander::visitIndexedAccess(IndexedAccessNode& node) {

		Node& rNode = *node.m_node;
		Node& rIndexNode = *node.m_indexNode;

		node.m_node = getReplacement(
			std::move(node.m_node),
			BEA_OK(None, visit(rNode))
		);
		node.m_indexNode = getReplacement(
			std::move(node.m_indexNode),
			BEA_OK(None, visit(rIndexNode))
		);

		/* cast<std::operator::Index<K, V>>(node) */
		VariableInfo *variable =
			&node.m_atom.operationImpl->type->type->
			object.publicMembers.data()[0].second;

		auto castTypeNode = std::make_unique<TypeNode>("Index");
		castTypeNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		auto castNode = std::make_unique<CastNode>(
			std::move(castTypeNode),
			std::move(node.m_node)
		);
		castNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.operationImpl->type
		);
		castNode->m_atom.flagDynamic = true;

		/* .get(index) */
		auto memberNode = std::make_unique<NamedAccessNode>(
			std::move(castNode), "get"
		);
		auto callNode = std::make_unique<CallNode>(
			std::move(memberNode)
		);
		callNode->m_argumentNodes.emplace_back(
			std::move(node.m_indexNode)
		);
		callNode->m_atom.type = std::make_unique<TypeID>(
			*node.m_atom.type
		);
		callNode->m_atom.variable = variable;

		m_optimizedNodes.emplace_back(std::move(callNode));
		return Mode::Replace;
	}

	/* Visit "if" statement */
	Result<None> ImplicitExpander::visitIfStatement(IfStatementNode& node) {

		for (Pair<std::unique_ptr<Node>>& pair: node.m_conditionalClauseNodes) {

			Node& rConditionNode = *pair[0];
			Node& rBodyNode = *pair[1];

			pair[0] = getReplacement(
				std::move(pair[0]),
				BEA_OK(None, visit(rConditionNode))
			);
			pair[1] = getReplacement(
				std::move(pair[1]),
				BEA_OK(None, visit(rBodyNode))
			);

			/* Fix condition */
			Type *type = pair[0]->m_atom.type->type;
			if (type->base != BaseType::Object &&
			    type->base != BaseType::U8) {

				auto castTypeNode = std::make_unique<TypeNode>("Boolean");
				castTypeNode->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				pair[0] = std::make_unique<CastNode>(
					std::move(castTypeNode),
					std::move(pair[0])
				);
				pair[0]->m_atom.type = std::make_unique<TypeID>(
					m_context.m_booleanType
				);
				continue;
			}
			else if (type->base != BaseType::Object)
				continue;

			pair[0] = makeTrueCast(std::move(pair[0]));
		}

		/* Else clause */
		if (node.m_elseClauseNode) {

			Node& rElseClauseNode = *node.m_elseClauseNode;
			node.m_elseClauseNode = getReplacement(
				std::move(node.m_elseClauseNode),
				BEA_OK(None, visit(rElseClauseNode))
			);
		}
		return Mode::Nothing;
	}

	/* Visit "while" statement */
	Result<None> ImplicitExpander::visitWhileStatement(WhileStatementNode& node) {

		Node& rConditionNode = *node.m_conditionNode;
		Node& rBodyNode = *node.m_bodyNode;

		node.m_conditionNode = getReplacement(
			std::move(node.m_conditionNode),
			BEA_OK(None, visit(rConditionNode))
		);
		node.m_bodyNode = getReplacement(
			std::move(node.m_bodyNode),
			BEA_OK(None, visit(rBodyNode))
		);

		/* Fix condition */
		Type *type = node.m_conditionNode->m_atom.type->type;
		if (type->base != BaseType::Object &&
		    type->base != BaseType::U8) {

			auto castTypeNode = std::make_unique<TypeNode>("Boolean");
			castTypeNode->m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
			node.m_conditionNode = std::make_unique<CastNode>(
				std::move(castTypeNode),
				std::move(node.m_conditionNode)
			);
			node.m_conditionNode->m_atom.type = std::make_unique<TypeID>(
				m_context.m_booleanType
			);
			return Mode::Nothing;
		}
		else if (type->base != BaseType::Object)
			return Mode::Nothing;

		node.m_conditionNode = makeTrueCast(
			std::move(node.m_conditionNode)
		);
		return Mode::Nothing;
	}

	/* Visit "return" statement */
	Result<None> ImplicitExpander::visitReturnStatement(ReturnStatementNode& node) {

		Node& rNode = *node.m_node;

		node.m_node = BEA_OK(None, makeExplicitCast(
			*node.m_atom.type,
			getReplacement(
				std::move(node.m_node),
				BEA_OK(None, visit(rNode))
			)
		));
		return Mode::Nothing;
	}

	/* Visit "let" statement */
	Result<None> ImplicitExpander::visitLetStatement(LetStatementNode& node) {

		Node& rTypeNode = *node.m_typeNode;
		Node& rValueNode = *node.m_valueNode;

		if (node.m_typeNode)
			node.m_typeNode = getReplacement(
				std::move(node.m_typeNode),
				BEA_OK(None, visit(rTypeNode))
			);
		if (node.m_valueNode)
			node.m_valueNode = BEA_OK(None, makeExplicitCast(
				*node.m_atom.type,
				getReplacement(
					std::move(node.m_valueNode),
					BEA_OK(None, visit(rValueNode))
				)
			));
		return Mode::Nothing;
	}

	/* Make explicit cast of node */
	Result<std::unique_ptr<Node>> ImplicitExpander::makeExplicitCast(
		const TypeID& typeID,
		std::unique_ptr<Node> node
	) {
		TypeID& otherTypeID = *node->m_atom.type;

		Type *leftType = typeID.type;
		Type *rightType = node->m_atom.type->type;

		if (!leftType || !rightType)
			return node;

		/* Object types */
		if (leftType->base == BaseType::Object) {

			if (leftType == rightType)
				return node;

			/* Check for type coercion */
			for (
				std::pair<Name, VariableInfo>& member:
				leftType->object.publicMembers
			) {
				Type *fType = member.second.type->type;

				if (member.first != "init" || !fType ||
				    fType->base != BaseType::Function ||
				    fType->function.parameters.size() != 1)
					continue;

				TypeID& gTypeID =
					*fType->function.parameters.data()[0].
					second.type;

				std::unique_ptr<TypeID> compareTypeID =
					substituteTypes(
						gTypeID,
						leftType->object.genericParameters,
						typeID.generics
					);
				if (compareTypeID->equalTo(otherTypeID) ==
				    TypeEquality::Equivalent) {

					/*
					 * Create "new" expression
					 *
					 * x -> new Type(x)
					 */
					auto newTypeNode = std::make_unique<TypeNode>(
						typeID.name
					);
					newTypeNode->m_atom.type =
						std::make_unique<TypeID>(
							typeID
						);
					auto newNode = std::make_unique<NewNode>(
						std::move(newTypeNode)
					);
					newNode->m_atom.type =
						std::make_unique<TypeID>(
							typeID
						);
					newNode->m_atom.variable = &member.second;
					newNode->m_argumentNodes.emplace_back(
						std::move(node)
					);
					newNode->m_atom.flagExpanded = true;

					return Result<std::unique_ptr<Node>>(
						std::move(newNode)
					);
				}
			}

			/* Check for implementation */
			for (
				const ObjectImplementation& implementation:
				rightType->object.implementations
			) {
				if (implementation.type->equalTo(typeID) !=
				    TypeEquality::Equivalent)
					continue;

				/*
				 * Create "cast" expression
				 *
				 * x -> cast<Type>(x)
				 */
				auto castTypeNode = std::make_unique<TypeNode>(
					typeID.name
				);
				castTypeNode->m_atom.type =
					std::make_unique<TypeID>(
						typeID
					);
				auto castNode = std::make_unique<CastNode>(
					std::move(castTypeNode),
					std::move(node)
				);
				castNode->m_atom.type =
					std::make_unique<TypeID>(
						typeID
					);
				castNode->m_atom.flagDynamic = true;
				castNode->m_atom.flagExpanded = true;

				return Result<std::unique_ptr<Node>>(
					std::move(castNode)
				);
			}
		}

		/* Cast basic types */
		else if (leftType->base != rightType->base) {

			auto castTypeNode = std::make_unique<TypeNode>(
				typeID.name
			);
			castTypeNode->m_atom.type =
				std::make_unique<TypeID>(
					typeID
				);
			auto castNode = std::make_unique<CastNode>(
				std::move(castTypeNode),
				std::move(node)
			);
			castNode->m_atom.type =
				std::make_unique<TypeID>(
					typeID
				);
			castNode->m_atom.flagDynamic = false;
			castNode->m_atom.flagExpanded = true;

			return Result<std::unique_ptr<Node>>(
				std::move(castNode)
			);
		}
		return node;
	}

	/* Make true cast of node */
	std::unique_ptr<Node> ImplicitExpander::makeTrueCast(
		std::unique_ptr<Node> node
	) {
		auto castTypeNode = std::make_unique<TypeNode>("True");
		castTypeNode->m_atom.type = std::make_unique<TypeID>(
			m_context.getType({"std", "operator", "True"}).value
		);
		TypeNode& pCastTypeNode = *castTypeNode;

		auto castNode = std::make_unique<CastNode>(
			std::move(castTypeNode),
			std::move(node)
		);
		castNode->m_atom.type = std::make_unique<TypeID>(
			*pCastTypeNode.m_atom.type
		);
		castNode->m_atom.flagDynamic = true;

		/* .isTrue() */
		auto memberNode = std::make_unique<NamedAccessNode>(
			std::move(castNode), "isTrue"
		);
		auto callNode = std::make_unique<CallNode>(std::move(memberNode));

		callNode->m_atom.type = std::make_unique<TypeID>(
			m_context.m_booleanType
		);
		callNode->m_atom.variable =
			pCastTypeNode.m_atom.type->type->object.
			publicMembers.get("isTrue").value;

		return std::move(callNode);
	}
}
