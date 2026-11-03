#include <format>
#include <bea/core.hpp>
#include <bea/analyzer/globalresolver.hpp>

namespace bea {

	/* Analyze node */
	Result<None> GlobalResolver::analyze(Node& node) {

		BEA_OK(None, visit(node));

		/* Serialize variable information */
		Frame& frame = *m_context.m_baseFrame;
		for (std::pair<std::string, VariableInfo>& pair: frame) {

			//std::cout << pair.first << ": " << *pair.second.type << std::endl;

			/* NOTE: Update for function pointers */
			if (pair.second.type->type &&
			    pair.second.type->type->base == BaseType::Function)
				m_serializer.writeFunction(pair.first, pair.second);
		}

		/* Serialize type information */
		for (std::pair<const Name, std::unique_ptr<Type>>& pair: frame.m_types) {

			if (!pair.second->external)
				continue;
			//std::cout << pair.first << *pair.second << std::endl;

			if (pair.second->base == BaseType::Object)
				m_serializer.writeClass(pair.first, *pair.second);
		}
		//std::cin.get();

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

	/* Visit "use" statement */
	Result<None> GlobalResolver::visitUseStatement(UseStatementNode& node) {

		Name alias = node.m_alias;
		if (alias.empty())
			alias =
				{node.m_name.components[
					node.m_name.components.size()-1
				]};

		m_serializer.writeUse(node.m_name, alias);
		return Result<None>();
	}

	/* Visit "func" statement (common) */
	Result<None> GlobalResolver::visitFuncStatement(FuncStatementNode& node) {

		VariableInfo& info =
			m_context.m_currentFrame->newEntry(node.m_name.components[0]);

		std::size_t& overloadCount =
			m_context.m_currentFrame->getOverloadCount(
				node.m_name.components[0]
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
				node.m_name.components[0],
				info.overloadID
			)
		};

		std::unique_ptr<Type> type = std::make_unique<Type>(
			BaseType::Function, typeName.components[0].c_str()
		);
		info.type = std::make_unique<TypeID>(typeName);
		info.type->type = type.get();

		/* Add parameters and return value */
		if (node.m_returnTypeNode)
			type->function.returnType =
				BEA_OK(None, nodeToTypeID(
					node.m_returnTypeNode.get()
				));
		else
			type->function.returnType =
				std::make_unique<TypeID>("None");

		for (const NodeFuncParameter& parameter: node.m_parameters) {

			VariableInfo& parameterInfo =
				type->function.parameters.newEntry(
					parameter.name.components[0]
				);
			parameterInfo.type = BEA_OK(None, nodeToTypeID(parameter.typeNode.get()));
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

				if (parameter.defaultNode)
					generic.substitutedType =
						BEA_OK(None, nodeToTypeID(
							parameter.defaultNode.get())
						);
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

		type->external = true;
		m_context.registerType(typeName.components[0], std::move(type));
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> GlobalResolver::visitFuncExprStatement(FuncExprStatementNode& node) {

		return visitFuncStatement(node);
	}

	/* Visit "func" statement with block body */
	Result<None> GlobalResolver::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		return visitFuncStatement(node);
	}

	/* Visit class field statement */
	Result<None> GlobalResolver::visitClassFieldStatement(ClassFieldStatementNode& node) {

		Result<VariableInfo *> result =
			m_context.m_currentFrame->get(node.m_name.components[0]);
		if (result.okay)
			return Result<None>(
				node.m_position,
				std::format(
					"Can't redeclare '{}'",
					node.m_name.components[0]
				)
			);
		VariableInfo& info =
			m_context.m_currentFrame->newEntry(node.m_name.components[0]);

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
		for (std::pair<std::string, VariableInfo>& pair: frame) {

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
					node.m_name.components[0]
				)
			);
		std::unique_ptr<Type> type = std::make_unique<Type>(node.m_name);
		if (node.m_abstract)
			type->object.flagAbstract = true;
		if (node.m_public)
			type->object.flagPublic = true;

		Frame& frame = *m_context.pushFrame();

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
			type->object.implementations.emplace_back(
				std::move(implementation)
			);
		}
		m_blockLevel--;

		/* Move members */
		for (std::pair<std::string, VariableInfo>& pair: frame) {

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

				if (parameter.defaultNode)
					generic.substitutedType =
						BEA_OK(None, nodeToTypeID(
							parameter.defaultNode.get())
						);
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
		typeID->type = m_context.m_baseFrame->getType(
			typeNode->getName()
		).value;

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
}
