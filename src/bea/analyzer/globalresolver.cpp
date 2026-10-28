#include <format>
#include <bea/core.hpp>
#include <bea/analyzer/globalresolver.hpp>

namespace bea {

	/* Create type identifier for type node */
	Result<std::unique_ptr<TypeID>> nodeToTypeID(Node& node) {

		if (!&node) return Result<std::unique_ptr<TypeID>>();

		TypeNode *typeNode = dynamic_cast<TypeNode *>(&node);
		if (!typeNode)
			return Result<std::unique_ptr<TypeID>>::fail();

		std::unique_ptr<TypeID> typeID = std::make_unique<TypeID>(
			typeNode->getName()
		);
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
					std::unique_ptr<TypeID>, nodeToTypeID(argument)
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

	/* Analyze node */
	Result<None> GlobalResolver::analyze(Node& node) {

		BEA_OK(None, visit(node));

		const Frame& frame = *m_context.m_baseFrame;
		for (const std::pair<std::string, VariableInfo>& pair: frame)
			std::cout << pair.first << ": " << *pair.second.type << std::endl;

		std::cin.get();

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

		m_serializer.writeU8((std::uint8_t)GRCode::Use);
		m_serializer.writeName(node.m_name);
		m_serializer.writeName(alias);

		return Result<None>();
	}

	/* Visit "func" statement (common) */
	Result<None> GlobalResolver::visitFuncStatement(FuncStatementNode& node) {

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

		info.flagUseDefault = node.m_useDefault;
		info.flagAbstract = node.m_abstract;
		info.flagPublic = node.m_public;

		/* Create type */
		Name typeName = {std::format("::{}", node.m_name.components[0])};

		std::unique_ptr<Type> type = std::make_unique<Type>(
			BaseType::Function, typeName.components[0].c_str()
		);
		info.type = std::make_unique<TypeID>(typeName);
		info.type->type = type.get();

		/* Add parameters and return value */
		type->function.returnType =
			BEA_OK(None, nodeToTypeID(*node.m_returnTypeNode));

		for (const NodeFuncParameter& parameter: node.m_parameters) {

			VariableInfo& parameterInfo =
				type->function.parameters.newEntry(
					parameter.name.components[0]
				);
			parameterInfo.type = BEA_OK(None, nodeToTypeID(*parameter.typeNode));
		}

		/* Add generic parameters */
		std::size_t i = 0;
		type->function.genericParameters.resize(node.m_genericParameters.size());

		for (const NodeGenericParameter& parameter: node.m_genericParameters) {

			GenericInfo& generic = type->function.genericParameters[i];

			generic.name = parameter.name.components[0];
			generic.type = parameter.type;

			/* Type parameter */
			if (parameter.type == GenericType::Type)
				generic.substitutedType =
					BEA_OK(None, nodeToTypeID(*parameter.defaultNode));

			/* Integer parameter */
			else if (parameter.type == GenericType::Int) {

				IntegerNode *intNode =
					dynamic_cast<IntegerNode *>(
						parameter.defaultNode.get()
					);
				generic.substitutedValue = intNode->m_value;
			}
		}

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
}
