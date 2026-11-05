#include <format>
#include <bea/core.hpp>
#include <bea/analyzer/globalresolver.hpp>

namespace bea {

	/* Analyze node */
	Result<None> GlobalResolver::analyze(Node& node) {

		BEA_OK(None, visit(node));

		BEA_OK(None, m_context.resolveAliases());
		BEA_OK(None, m_context.resolveReferences());

		/* Serialize aliases */
		for (const std::pair<Name, Name>& pair: m_context.m_aliases)
			m_serializer.writeUse(pair.second, pair.first);

		/* Serialize variable information */
		Frame& frame = *m_context.m_baseFrame;
		for (std::pair<Name, VariableInfo>& pair: frame) {

			/* TODO: Update for function pointers */
			if (pair.second.type->type &&
			    pair.second.type->type->base == BaseType::Function)
				m_serializer.writeFunction(pair.first, pair.second);
		}

		/* Serialize type information */
		for (std::pair<const Name, std::unique_ptr<Type>>& pair: frame.m_types) {

			if (!pair.second->external)
				continue;

			if (pair.second->base == BaseType::Object)
				m_serializer.writeClass(pair.first, *pair.second);
		}

		/* Serialize external references */
		for (const Context::Reference& reference: m_context.m_externalReferences) {

			if (reference.resolved)
				continue;

			GRCode type = GRCode::ExternalType;
			if (reference.type == Context::ReferenceType::Entity)
				type = GRCode::ExternalEntity;
			m_serializer.writeExternal(type, reference.name);
		}
		return Result<None>();
	}

	/* Visit block */
	Result<None> GlobalResolver::visitBlock(BlockNode& node) {

		m_blockLevel++;

		for (std::size_t i = 0; i < node.m_nodes.size(); i++)
			BEA_OK(None, visit(*node.m_nodes[i]));

		m_blockLevel--;
		return Result<None>();
	}

	/* Visit type */
	Result<None> GlobalResolver::visitType(TypeNode& node) {

		Result<Type *> result =
			m_context.getType(
				node.m_name, true
			);
		if (!result.okay)
			registerTypeReference(node.m_name, node, nullptr);

		return Result<None>();
	}

	/* Visit variable access */
	Result<None> GlobalResolver::visitVariableAccess(VariableAccessNode& node) {

		Result<VariableInfo *> result =
			m_context.m_currentFrame->get(
				node.m_name, true
			);
		if (!result.okay &&
		    node.m_name != "true" &&
		    node.m_name != "false" &&
		    node.m_name != "null")
			registerEntityReference(node.m_name, node);

		return Result<None>();
	}

	/* Visit "new" expression */
	Result<None> GlobalResolver::visitNew(NewNode& node) {

		std::unique_ptr<TypeID> typeID =
			BEA_OK(None, nodeToTypeID(node.m_typeNode.get()));
		if (!typeID->type) {

			TypeNode *typeNode =
				static_cast<TypeNode *>(
					node.m_typeNode.get()
				);
			registerTypeReference(
				typeNode->m_name,
				*typeNode,
				nullptr
			);
		}

		for (const std::unique_ptr<Node>& argument: node.m_argumentNodes)
			BEA_OK(None, visit(*argument));

		return Result<None>();
	}

	/* Visit "cast" expression */
	Result<None> GlobalResolver::visitCast(CastNode& node) {

		std::unique_ptr<TypeID> typeID =
			BEA_OK(None, nodeToTypeID(node.m_typeNode.get()));
		if (!typeID->type) {

			TypeNode *typeNode =
				static_cast<TypeNode *>(
					node.m_typeNode.get()
				);
			registerTypeReference(
				typeNode->m_name,
				*typeNode,
				nullptr
			);
		}
		BEA_OK(None, visit(*node.m_valueNode));

		return Result<None>();
	}

	/* Visit "if" statement */
	Result<None> GlobalResolver::visitIfStatement(IfStatementNode& node) {

		for (Pair<std::unique_ptr<Node>>& pair: node.m_conditionalClauseNodes) {

			m_context.pushFrame();

			BEA_OK(None, visit(*pair[0]));
			BEA_OK(None, visit(*pair[1]));

			m_context.popFrame();
		}
		if (node.m_elseClauseNode) {

			m_context.pushFrame();
			BEA_OK(None, visit(*node.m_elseClauseNode));
			m_context.popFrame();
		}
		return Result<None>();
	}

	/* Visit "case" statement */
	Result<None> GlobalResolver::visitCaseStatement(CaseStatementNode& node) {

		m_context.pushFrame();

		BEA_OK(None, visit(*node.m_caseNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "case let" statement */
	Result<None> GlobalResolver::visitCaseLetStatement(CaseLetStatementNode& node) {

		Frame& frame = *m_context.pushFrame();
		VariableInfo& variable = frame[node.m_name];

		BEA_OK(None, visit(*node.m_caseNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "which" statement */
	Result<None> GlobalResolver::visitWhichStatement(WhichStatementNode& node) {

		m_context.pushFrame();

		BEA_OK(None, visit(*node.m_valueNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "for" statement */
	Result<None> GlobalResolver::visitForStatement(ForStatementNode& node) {

		Frame& frame = *m_context.pushFrame();
		VariableInfo& variable = frame[node.m_name];

		BEA_OK(None, visit(*node.m_iterableNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "while" statement */
	Result<None> GlobalResolver::visitWhileStatement(WhileStatementNode& node) {

		m_context.pushFrame();

		BEA_OK(None, visit(*node.m_conditionNode));
		BEA_OK(None, visit(*node.m_bodyNode));

		m_context.popFrame();
		return Result<None>();
	}

	/* Visit "use" statement */
	Result<None> GlobalResolver::visitUseStatement(UseStatementNode& node) {

		Name alias = node.m_alias;
		if (alias.empty())
			alias = {
				node.m_name.components[
					node.m_name.components.size()-1
				],
			};
		Name& newName = m_context.m_aliases[alias];
		if (!newName.empty())
			return Result<None>(
				node.m_position,
				std::format(
					"Can't redeclare alias '{}'",
					alias
				)
			);
		newName = node.m_name;

		return Result<None>();
	}

	/* Visit "let" statement */
	Result<None> GlobalResolver::visitLetStatement(LetStatementNode& node) {

		if (!node.m_typeNode && m_blockLevel == 1)
			return Result<None>(
				node.m_position,
				"Global declaration can't have inferred type"
			);

		BEA_OK(None, visit(*node.m_valueNode));

		/* Declare variables */
		for (const Name& name: node.m_names) {

			Result<VariableInfo *> result =
				m_context.m_currentFrame->get(name);
			if (result.okay)
				return Result<None>(
					node.m_position,
					std::format(
						"Can't redeclare '{}'",
						name
					)
				);
			VariableInfo& info =
				m_context.m_currentFrame->newEntry(name);

			info.flagPublic = node.m_public;
			if (node.m_typeNode)
				info.type = BEA_OK(
					None, nodeToTypeID(node.m_typeNode.get())
				);
		}
		return Result<None>();
	}

	/* Visit "func" statement (common) */
	Result<None> GlobalResolver::visitFuncStatement(
		FuncStatementNode& node,
		Node& bodyNode
	) {

		VariableInfo& info =
			m_context.m_currentFrame->newEntry(node.m_name);

		std::size_t& overloadCount =
			m_context.m_currentFrame->getOverloadCount(
				node.m_name
			);

		info.flagUseDefault = node.m_useDefault;
		info.flagAbstract = node.m_abstract;
		info.flagPublic = node.m_public;
		info.flagStatic = node.m_static;
		info.overloadID = overloadCount++;

		/* Create type */
		Name typeName = {
			std::format(
				"::{}::{}",
				node.m_name,
				info.overloadID
			)
		};

		std::unique_ptr<Type> type = std::make_unique<Type>(
			BaseType::Function, typeName
		);
		info.type = std::make_unique<TypeID>(typeName);
		info.type->type = type.get();

		/* Add parameters and return value */
		if (node.m_returnTypeNode) {

			type->function.returnType =
				BEA_OK(None, nodeToTypeID(
					node.m_returnTypeNode.get()
				));
			if (!type->function.returnType->type)
				registerTypeReference(
					type->function.returnType->name,
					*node.m_returnTypeNode,
					&type->function.returnType->type
				);
		}
		else type->function.returnType =
			std::make_unique<TypeID>("None");

		for (const NodeFuncParameter& parameter: node.m_parameters) {

			VariableInfo& parameterInfo =
				type->function.parameters.newEntry(
					parameter.name
				);
			parameterInfo.type = BEA_OK(
				None, nodeToTypeID(parameter.typeNode.get())
			);
			if (!parameterInfo.type->type)
				registerTypeReference(
					parameterInfo.type->name,
					*parameter.typeNode,
					&parameterInfo.type->type
				);
		}

		/* Add generic parameters */
		std::size_t i = 0;
		type->function.genericParameters.resize(node.m_genericParameters.size());

		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			GenericInfo& generic = type->function.genericParameters[i];

			generic.name = parameter.name.components[0];
			generic.type = parameter.type;

			/* Type parameter */
			if (parameter.type == GenericType::Type) {

				if (parameter.defaultNode) {

					generic.substitutedType =
						BEA_OK(None, nodeToTypeID(
							parameter.defaultNode.get())
						);
					if (!generic.substitutedType->type)
						registerTypeReference(
							generic.substitutedType->name,
							*parameter.defaultNode,
							&generic.substitutedType->type
						);
				}
			}

			/* Integer parameter */
			else if (parameter.type == GenericType::Int) {

				IntegerNode *intNode =
					dynamic_cast<IntegerNode *>(
						parameter.defaultNode.get()
					);
				if (intNode)
					generic.substitutedValue = intNode->m_value;
			}
		}

		/* Create temporary constants for generic parameters */
		Frame& frame = *m_context.pushFrame();
		VariableInfo& thisVariable = frame["this"];

		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			if (parameter.type == GenericType::Type) {

				std::unique_ptr<Type>& type =
					frame.m_types[parameter.name];
				type = std::make_unique<Type>(
					BaseType::Any,
					parameter.name
				);
			}
			else if (parameter.type == GenericType::Int)
				frame.newEntry(parameter.name);
		}

		/* Create temporary variables for parameters */
		for (const NodeFuncParameter& parameter: node.m_parameters)
			frame.newEntry(parameter.name);

		/* Visit body */
		BEA_OK(None, visit(bodyNode));
		m_context.popFrame();

		type->external = true;
		m_context.registerType(typeName, std::move(type));
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> GlobalResolver::visitFuncExprStatement(FuncExprStatementNode& node) {

		return visitFuncStatement(node, *node.m_bodyNode);
	}

	/* Visit "func" statement with block body */
	Result<None> GlobalResolver::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		return visitFuncStatement(node, *node.m_bodyNode);
	}

	/* Visit class field statement */
	Result<None> GlobalResolver::visitClassFieldStatement(ClassFieldStatementNode& node) {

		Result<VariableInfo *> result =
			m_context.m_currentFrame->get(node.m_name);
		if (result.okay)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't redeclare '{}'",
					node.m_name
				)
			);
		VariableInfo& info =
			m_context.m_currentFrame->newEntry(node.m_name);

		info.flagPublic = node.m_public;
		//TODO:info.flagStatic = node.m_static;
		info.type = BEA_OK(None, nodeToTypeID(node.m_typeNode.get()));

		return Result<None>();
	}

	/* Visit "impl" statement */
	Result<None> GlobalResolver::visitImplStatement(ImplStatementNode& node) {

		Frame& frame = *m_context.pushFrame();

		BEA_OK(None, visit(*node.m_bodyNode));

		/* Move members */
		for (std::pair<Name, VariableInfo>& pair: frame) {

			VariableInfo& info =
				m_implMembers.newEntry(pair.first);
			info = std::move(pair.second);
		}
		m_implTypes = std::move(frame.m_types);
		m_context.popFrame();

		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> GlobalResolver::visitClassStatement(ClassStatementNode& node) {

		Result<Type *> result = m_context.getType(node.m_name);
		if (result.okay)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't redeclare type '{}'",
					node.m_name
				)
			);
		std::unique_ptr<Type> type = std::make_unique<Type>(node.m_name);
		if (node.m_abstract)
			type->object.flagAbstract = true;
		if (node.m_public)
			type->object.flagPublic = true;

		Frame& frame = *m_context.pushFrame();

		/* Create temporary constants for generic parameters */
		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

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
			else if (parameter.type == GenericType::Int)
				frame.newEntry(parameter.name);
		}

		/* Visit nodes */
		BlockNode *block = static_cast<BlockNode *>(node.m_bodyNode.get());
		m_blockLevel++;

		for (std::size_t i = 0; i < block->m_nodes.size(); i++) {

			/* Create implementation info */
			BEA_OK(None, visit(*block->m_nodes[i]));
			ImplStatementNode *implNode =
				dynamic_cast<ImplStatementNode *>(block->m_nodes[i].get());

			if (!implNode) continue;

			ObjectImplementation implementation = {
				.type = BEA_OK(None, nodeToTypeID(implNode->m_typeNode.get())),
				.members = std::move(m_implMembers),
				.internalTypes = std::move(m_implTypes),
			};
			if (!implementation.type->type)
				registerTypeReference(
					implementation.type->name,
					*implNode->m_typeNode,
					&implementation.type->type
				);

			type->object.implementations.emplace_back(
				std::move(implementation)
			);
		}
		m_blockLevel--;

		/* Move members */
		for (std::pair<Name, VariableInfo>& pair: frame) {

			if (pair.second.flagPublic) {

				VariableInfo& info =
					type->object.publicMembers.newEntry(pair.first);
				info = std::move(pair.second);
			}
			else {
				VariableInfo& info =
					type->object.privateMembers.newEntry(pair.first);
				info = std::move(pair.second);
			}
		}
		type->object.internalTypes = std::move(frame.m_types);

		/* Add generic parameters */
		std::size_t i = 0;
		type->object.genericParameters.resize(node.m_genericParameters.size());

		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			GenericInfo& generic = type->object.genericParameters[i];

			generic.name = parameter.name.components[0];
			generic.type = parameter.type;

			/* Type parameter */
			if (parameter.type == GenericType::Type) {

				if (parameter.defaultNode) {

					generic.substitutedType =
						BEA_OK(None, nodeToTypeID(
							parameter.defaultNode.get())
						);
					if (!generic.substitutedType->type)
						registerTypeReference(
							generic.substitutedType->name,
							*parameter.defaultNode,
							&generic.substitutedType->type
						);
				}
			}

			/* Integer parameter */
			else if (parameter.type == GenericType::Int) {

				IntegerNode *intNode =
					dynamic_cast<IntegerNode *>(
						parameter.defaultNode.get()
					);
				if (intNode)
					generic.substitutedValue = intNode->m_value;
			}
		}
		m_context.popFrame();

		type->external = true;
		m_context.registerType(node.m_name, std::move(type));

		return Result<None>();
	}

	/* Create type identifier for type node */
	Result<std::unique_ptr<TypeID>> GlobalResolver::nodeToTypeID(Node *node) {

		if (!node) return Result<std::unique_ptr<TypeID>>();

		TypeNode *typeNode = dynamic_cast<TypeNode *>(node);
		if (!typeNode)
			return Result<std::unique_ptr<TypeID>>::fail();

		std::unique_ptr<TypeID> typeID = std::make_unique<TypeID>(
			typeNode->getName()
		);
		Result<Type *> typeResult = m_context.getType(
			typeNode->getName(), true
		);
		if (typeResult.okay)
			typeID->type = typeResult.value;

		typeID->generics.resize(typeNode->m_genericArgumentNodes.size());
		for (std::size_t i = 0; i < typeNode->m_genericArgumentNodes.size(); i++) {

			Node& argument = *typeNode->m_genericArgumentNodes[i];
			GenericInfo& generic = typeID->generics[i];

			/* Type generic */
			TypeNode *genericTypeNode =
				dynamic_cast<TypeNode *>(&argument);
			if (genericTypeNode) {

				generic.type = GenericType::Type;
				generic.substitutedType = BEA_OK(
					std::unique_ptr<TypeID>, nodeToTypeID(&argument)
				);
			}

			/* Integer generic */
			IntegerNode *genericIntNode =
				dynamic_cast<IntegerNode *>(&argument);
			if (genericIntNode) {

				generic.type = GenericType::Int;
				generic.substitutedValue = genericIntNode->m_value;
			}
		}
		return std::move(typeID);
	}

	/* Register type reference */
	void GlobalResolver::registerTypeReference(
		Name& name,
		Node& node,
		Type **pType
	) {
		Context::Reference reference = {
			.type = Context::ReferenceType::Type,
			.name = name,
			.node = node,
			.pType = pType,
		};
		m_context.m_externalReferences.emplace_back(std::move(reference));
	}

	/* Register entity reference */
	void GlobalResolver::registerEntityReference(
		Name& name,
		Node& node
	) {
		Context::Reference reference = {
			.type = Context::ReferenceType::Entity,
			.name = name,
			.node = node,
		};
		m_context.m_externalReferences.emplace_back(std::move(reference));
	}
}
