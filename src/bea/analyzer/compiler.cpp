/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bea/core.hpp>
#include <bea/hash.hpp>
#include <bea/analyzer/compiler.hpp>

namespace bea {

	/* Analyze node */
	Result<None> Compiler::analyze(Node& node) {

		BEA_OK(None, visit(node));

		m_stream << m_globalStream.str();
		m_globalStream.str("");

		m_stream <<
			"$" << SerialName(m_context.m_moduleName) <<
			"____init :: proc() u64 {\n";

		m_stream << m_initStream.str();
		m_initStream.str("");

		m_stream << "}\n";
		return Result<None>();
	}

	/* Visit block */
	Result<None> Compiler::visitBlock(BlockNode& node) {

		m_blockLevel++;

		bool noPushBlockScope = m_noPushBlockScope;
		m_noPushBlockScope = false;

		if (m_blockLevel == 1)
			noPushBlockScope = true;

		if (!noPushBlockScope)
			pushScope();

		/* Visit nodes */
		scopeStream() << "{\n";

		bool lastIsReturn = false;
		bool isLast = false;
		bool keepRef = false;
		Type *lastType = nullptr;

		std::size_t i = 0;
		for (std::unique_ptr<Node>& statement: node.m_nodes) {

			if (++i == node.m_nodes.size())
				isLast = true;

			lastIsReturn = false;
			if (dynamic_cast<ReturnStatementNode *>(statement.get()))
				lastIsReturn = true;

			bool deleteRef = false;
			bool deleteAnyRef = false;

			/* Figure out if we need to try delete object */
			if (statement->m_atom.flagExpression) {

				Type *type = statement->m_atom.type->type;
				if (!isLast) {

					if (type->base == BaseType::Object)
						deleteRef = true;
					else if (type->base == BaseType::Any)
						deleteAnyRef = true;
				}
				else {
					keepRef = true;
					lastType = type;
				}
			}
			if (deleteRef) scopeStream() << "$__std__delete(";
			if (deleteAnyRef) scopeStream() << "$__std__deleteAny(";
			if (keepRef) scopeStream() << "var %__keep = ";

			BEA_OK(None, visit(*statement));

			if (deleteRef) scopeStream() << ")u64";
			if (deleteAnyRef) scopeStream() << ")u64";

			scopeStream() << '\n';
		}

		/* Write end of scope */
		if (keepRef && lastType->base == BaseType::Object)
			scopeStream() << "$__std__incref(%__keep)ptr\n";

		if (!lastIsReturn)
			writeScopeEnd(m_rcScope.size()-1);

		if (keepRef && lastType->base == BaseType::Object)
			scopeStream() << "$__std__decrefSafe(%__keep)ptr\n%__keep\n";

		scopeStream() << "}\n";

		if (!noPushBlockScope)
			popScope();
		m_blockLevel--;

		return Result<None>();
	}

	/* Visit integer constant */
	Result<None> Compiler::visitInteger(IntegerNode& node) {

		scopeStream() <<
			node.m_value <<
			baseType(node.m_atom.type->type->base);

		return Result<None>();
	}

	/* Visit float constant */
	Result<None> Compiler::visitFloat(FloatNode& node) {

		Type *type = node.m_atom.type->type;

		uint64_t value = *(uint64_t *)&node.m_value;
		if (type->base == BaseType::F32) {

			float fValue = (float)node.m_value;
			value = (uint64_t)*(uint32_t *)&fValue;
		}
		scopeStream() << value << baseType(type->base);

		return Result<None>();
	}

	/* Visit string constant */
	Result<None> Compiler::visitString(StringNode& node) {

		std::size_t id = m_stringID++;

		std::ostringstream nameStream;
		nameStream <<
			"$" << SerialName(m_context.m_moduleName) <<
			"____str" << id;

		std::string name = nameStream.str();
		nameStream.str("");

		/* Write string data */
		m_globalStream << name << "data :: ptr \"";
		for (std::size_t i = 0; i < node.m_value.length(); i++) {

			char c = node.m_value[i];

			if (c == '\n')
				m_globalStream << "\\n";
			else if (c == '"')
				m_globalStream << "\\\"";
			else if (c == '\\')
				m_globalStream << "\\\\";
			else
				m_globalStream << c;
		}
		m_globalStream << "\"u8\n" << name << " :: ptr {}\n";

		/* Write string object initializer */
		m_initStream <<
			"*ptr " << name << " = $__std__string__newconst(*ptr " <<
			name << "data)ptr\n";

		scopeStream() << "*ptr " << name;
		return Result<None>();
	}

	/* Visit variable access */
	Result<None> Compiler::visitVariableAccess(VariableAccessNode& node) {

		Type *type = node.m_atom.type->type;
		if (type->base != BaseType::Any && !node.m_atom.scopeID)
			scopeStream() << "*"  << baseType(type->base) << " ";

		if (node.m_atom.scopeID)
			scopeStream() << '%';
		else
			scopeStream() << '$';

		scopeStream() << node.m_name;
		return Result<None>();
	}

	/* Visit assignment */
	Result<None> Compiler::visitAssignment(AssignmentNode& node) {

		Type *type = node.m_atom.type->type;
		Type *leftType = node.m_leftNode->m_atom.type->type;

		NamedAccessNode *memberNode =
			dynamic_cast<NamedAccessNode *>(
				node.m_leftNode.get()
			);
		/*if (memberNode)
			leftType = memberNode->m_atom.variable->type->type;*/

		Type *rightType = node.m_rightNode->m_atom.type->type;

		/* Assign any type */
		if (leftType->base == BaseType::Any) {

			if (rightType->base != BaseType::Any) {

				scopeStream() << "{\nvar %__temp = [16] {}\n";

				scopeStream() <<
					"*u64 (%__temp + 0) = " <<
					(int)rightType->base << "u64\n";
				scopeStream() <<
					"*" << baseType(rightType->base) <<
					" (%__temp + 8) = ";

				BEA_OK(None, visit(*node.m_rightNode));
				scopeStream() << "\n";
			}
			scopeStream() << "$__std__decincAny(";
			BEA_OK(None, visit(*node.m_leftNode));
			scopeStream() << ", ";
			BEA_OK(None, visit(*node.m_leftNode));
			scopeStream() << ", ";

			if (rightType->base != BaseType::Any)
				scopeStream() << "%__temp";
			else
				BEA_OK(None, visit(*node.m_rightNode));

			scopeStream() << ")ptr";

			if (rightType->base != BaseType::Any)
				scopeStream() << "\n}";
		}

		/* Assign regular type */
		else {
			scopeStream() << "(";
			BEA_OK(None, visit(*node.m_leftNode));
			scopeStream() << " = " << baseType(leftType->base) << " ";

			if (leftType->base == BaseType::Object) {

				scopeStream() << "$__std__decinc(";
				BEA_OK(None, visit(*node.m_leftNode));
				scopeStream() << ", ";
			}

			if (rightType->base == BaseType::Any)
				scopeStream() <<
					"*" << baseType(leftType->base) <<
					" (";
			BEA_OK(None, visit(*node.m_rightNode));
			if (rightType->base == BaseType::Any)
				scopeStream() << " + 8)";

			if (leftType->base == BaseType::Object)
				scopeStream() << ")ptr";
			scopeStream() << ")";
		}
		return Result<None>();
	}

	/* Visit binary operation */
	Result<None> Compiler::visitBinaryOperation(BinaryOperationNode& node) {

		if (node.m_operation == "<=" || node.m_operation == ">=")
			scopeStream() << "(!";
		scopeStream() << "(";

		/* Operation */
		BEA_OK(None, visit(*node.m_leftNode));

		if (node.m_operation == "+")
			scopeStream() << " + ";
		else if (node.m_operation == "-")
			scopeStream() << " - ";
		else if (node.m_operation == "*")
			scopeStream() << " * ";
		else if (node.m_operation == "%")
			scopeStream() << " % ";
		else if (node.m_operation == "|")
			scopeStream() << " | ";
		else if (node.m_operation == "&")
			scopeStream() << " & ";
		else if (node.m_operation == "^")
			scopeStream() << " ^ ";
		else if (node.m_operation == "<<")
			scopeStream() << " << ";
		else if (node.m_operation == ">>")
			scopeStream() << " >> ";
		else if (node.m_operation == "==")
			scopeStream() << " == ";
		else if (node.m_operation == "!=")
			scopeStream() << " != ";
		else if (node.m_operation == "<")
			scopeStream() << " < ";
		else if (node.m_operation == ">")
			scopeStream() << " > ";
		else if (node.m_operation == "<=")
			scopeStream() << " > ";
		else if (node.m_operation == ">=")
			scopeStream() << " < ";
		else if (node.m_operation == "and")
			scopeStream() << " and ";
		else if (node.m_operation == "or")
			scopeStream() << " or ";

		BEA_OK(None, visit(*node.m_rightNode));

		scopeStream() << ")";
		if (node.m_operation == "<=" || node.m_operation == ">=")
			scopeStream() << ")";
		return Result<None>();
	}

	/* Visit "new" expression */
	Result<None> Compiler::visitNew(NewNode& node) {

		VariableInfo *variable = node.m_atom.variable;
		Type *type = variable->type->type;

		std::size_t anyCount = 0;

		/* Construct any containers for parameters that require it */
		std::size_t i = 0;
		for (std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			auto& parameter = type->function.parameters.data()[i++];

			Type *paramType = parameter.second.type->type;
			Type *argumentType = argument->m_atom.type->type;

			if (paramType->base != BaseType::Any ||
			    argumentType->base == BaseType::Any)
				continue;
			if (!anyCount++) scopeStream() << "{\n";

			/* Create container */
			scopeStream() << "var %__p" << (i-1) << " = [16] {}\n";
			scopeStream() <<
				"*u64 (%__p" << (i-1) << " + 0ptr) = " <<
				(int)argumentType->base << "u64\n";
			scopeStream() <<
				"*" << baseType(argumentType->base) <<
				" (%__p" << (i-1) << " + 8ptr) = ";

			BEA_OK(None, visit(*argument));
			scopeStream() << '\n';
		}
		scopeStream() <<
			"$" << SerialName(node.m_atom.type->name) <<
			"____new" << variable->overloadID << "(";

		/* Arguments */
		i = 0;
		for (std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			if (i++) scopeStream() << ", ";
			auto& parameter = type->function.parameters.data()[i-1];

			Type *paramType = parameter.second.type->type;
			Type *argumentType = argument->m_atom.type->type;

			if (paramType->base != BaseType::Any ||
			    argumentType->base == BaseType::Any) {

				BEA_OK(None, visit(*argument));
				continue;
			}
			scopeStream() << "%__p" << (i-1);
		}
		scopeStream() << ")ptr";

		if (anyCount) scopeStream() << "\n}";
		return Result<None>();
	}

	/* Visit "cast" expression */
	Result<None> Compiler::visitCast(CastNode& node) {

		TypeID& typeID = *node.m_atom.type;
		Type *type = typeID.type;

		if (type->base <= BaseType::F64) {

			scopeStream() << baseType(type->base) << " ";
			BEA_OK(None, visit(*node.m_valueNode));

			return Result<None>();
		}

		/* Cast object */
		Hash hash(typeID);

		scopeStream() << "$__std__cast(";
		BEA_OK(None, visit(*node.m_valueNode));
		scopeStream() << ", " << hash.getValue() << "u32)ptr";

		return Result<None>();
	}

	/* Visit function call */
	Result<None> Compiler::visitCall(CallNode& node) {

		VariableInfo *variable = node.m_atom.variable;
		Type *type = variable->type->type;

		Type *returnType = type->function.returnType->type;
		Type *resultType = node.m_atom.type->type;

		/* Determine function name */
		Name name{};
		Node *thisNode = nullptr;

		VariableAccessNode *varNode =
			dynamic_cast<VariableAccessNode *>(
				node.m_node.get()
			);
		NamedAccessNode *memberNode =
			dynamic_cast<NamedAccessNode *>(
				node.m_node.get()
			);
		if (varNode) name = varNode->m_name;

		else if (memberNode) {

			thisNode = memberNode->m_node.get();
			Type *classType = thisNode->m_atom.type->type;

			name = memberNode->m_name;
			name.components.insert(
				name.components.begin(),
				classType->name.components.begin(),
				classType->name.components.end()
			);
		}

		/* Any return value */
		std::size_t anyCount = 0;
		std::size_t iota = m_iota++;

		if (returnType->base == BaseType::Any) {

			scopeStream() <<
				"{\nvar %__return" << iota <<
				" = [16] {}\n";
			anyCount++;
		}

		/* Construct any containers for parameters that require it */
		std::size_t i = 0;
		for (std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			auto& parameter = type->function.parameters.data()[i++];

			Type *paramType = parameter.second.type->type;
			Type *argumentType = argument->m_atom.type->type;

			if (paramType->base != BaseType::Any ||
			    argumentType->base == BaseType::Any)
				continue;
			if (!anyCount++) scopeStream() << "{\n";

			/* Create container */
			scopeStream() << "var %__p" << (i-1) << " = [16] {}\n";
			scopeStream() <<
				"*u64 (%__p" << (i-1) << " + 0ptr) = " <<
				(int)argumentType->base << "u64\n";
			scopeStream() <<
				"*" << baseType(argumentType->base) <<
				" (%__p" << (i-1) << " + 8ptr) = ";

			BEA_OK(None, visit(*argument));
			scopeStream() << '\n';
		}

		/* Name */
		scopeStream() <<
			"$" << SerialName(name) << "____" <<
			variable->overloadID << "(";

		/* Arguments */
		i = 0;
		std::size_t j = 0;

		if (returnType->base == BaseType::Any) {

			scopeStream() << "%__return" << iota;
			j++;
		}
		if (thisNode) {

			if (j++) scopeStream() << ", ";
			BEA_OK(None, visit(*thisNode));
		}
		for (std::unique_ptr<Node>& argument: node.m_argumentNodes) {

			if (j++) scopeStream() << ", ";
			i++;

			auto& parameter = type->function.parameters.data()[i-1];

			Type *paramType = parameter.second.type->type;
			Type *argumentType = argument->m_atom.type->type;

			if (paramType->base != BaseType::Any ||
			    argumentType->base == BaseType::Any) {

				BEA_OK(None, visit(*argument));
				continue;
			}
			scopeStream() << "%__p" << (i-1);
		}
		scopeStream() << ")";

		/* Any return value */
		if (returnType->base == BaseType::Any &&
		    resultType->base != BaseType::Any) {

			scopeStream() <<
				"ptr\n*" << baseType(resultType->base) <<
				" (%__return" << iota << " + 8ptr)";
		}
		else scopeStream() << baseType(resultType->base);

		if (anyCount) scopeStream() << "\n}";
		return Result<None>();
	}

	/* Visit named member access */
	Result<None> Compiler::visitNamedAccess(NamedAccessNode& node) {

		VariableInfo *variable = node.m_atom.variable;
		Type *type = variable->type->type;
		Type *resultType = node.m_atom.type->type;

		if (type->base != BaseType::Any)
			scopeStream() << "*" << baseType(type->base) << " ";

		else if (resultType->base != BaseType::Any)
			scopeStream() <<
				"*" << baseType(resultType->base) <<
				" (";

		scopeStream() << "(";
		BEA_OK(None, visit(*node.m_node));
		scopeStream() << " + " << variable->position << "ptr)";

		if (type->base == BaseType::Any &&
		    resultType->base != BaseType::Any)
			scopeStream() << " + 8ptr)";

		return Result<None>();
	}

	/* Visit "return" statement */
	Result<None> Compiler::visitReturnStatement(ReturnStatementNode& node) {

		BEA_OK(None, writeReturnValue(*node.m_node));

		return Result<None>();
	}

	/* Visit "let" statement */
	Result<None> Compiler::visitLetStatement(LetStatementNode& node) {

		if (!node.m_atom.scopeID)
			m_useInitStream = true;

		Type *type = node.m_atom.type->type;
		for (const Name& letName: node.m_names) {

			std::ostringstream nameStream;
			if (node.m_atom.scopeID)
				nameStream << "%";
			else
				nameStream << "$";
			nameStream << SerialName(letName);

			std::string name = nameStream.str();
			nameStream.str("");

			/* Declare variable */
			if (node.m_atom.scopeID)
				m_stream << "var " << name << " = ";
			else
				m_stream << name << " :: ";

			if (type->base == BaseType::Any)
				m_stream << "[16]";
			else
				m_stream << baseType(type->base);
			m_stream << " {}\n";

			if (node.m_valueNode) {

				/* Assign any type */
				if (type->base == BaseType::Any) {

					scopeStream() <<
						"var %" << SerialName(letName) <<
						"____init = ptr ";
					BEA_OK(None, visit(*node.m_valueNode));
					scopeStream() << "\n";

					scopeStream() <<
						"*u64 (" << name <<
						" + 0) = *u64 (%" <<
						SerialName(letName) <<
						"____init + 0)\n";
					scopeStream() <<
						"*u64 (" << name <<
						" + 8) = *u64 (%" <<
						SerialName(letName) <<
						"____init + 8)\n";
					scopeStream() <<
						"$__std__increfAny(0ptr, " <<
						name << ")u64\n";
				}

				/* Assign regular value */
				else {
					if (!node.m_atom.scopeID)
						scopeStream() <<
							"*" << baseType(type->base) <<
							" ";

					scopeStream() << name << " = ";

					if (type->base == BaseType::Object)
						scopeStream() << "$__std__incref(";
					BEA_OK(None, visit(*node.m_valueNode));
					if (type->base == BaseType::Object)
						scopeStream() << ")ptr";

					scopeStream() << "\n";
				}
			}

			/* Add to scope */
			if ((type->base == BaseType::Object ||
			     type->base == BaseType::Any) &&
			    m_rcScope.size()) {

				std::pair<Name, BaseType> rc = {
					letName, type->base
				};
				m_rcScope[m_rcScope.size()-1].emplace_back(
					std::move(rc)
				);
			}
		}
		m_useInitStream = false;
		return Result<None>();
	}

	/* Visit "func" statement */
	Result<None> Compiler::visitFuncStatement(FuncStatementNode& node) {

		Name name = node.m_name;
		if (m_classType)
			name.components.insert(
				name.components.begin(),
				m_classType->name.components.begin(),
				m_classType->name.components.end()
			);
		Type *type = node.m_atom.type->type;
		Type *returnType = type->function.returnType->type;

		m_returnType = returnType;

		Hash implHash{};
		std::string implHashString{};
		VariableInfo *variable = nullptr;

		/* Locate variable */
		if (m_implementation) {

			implHash = Hash(*m_implementation->type);
			implHashString = implHash.toString();

			for (auto& pair: m_implementation->members) {

				if (pair.second.type->type != type)
					continue;
				variable = &pair.second;
				break;
			}
		}
		else if (m_classType) {

			for (auto& pair: m_classType->object.publicMembers) {

				if (pair.second.type->type != type)
					continue;
				variable = &pair.second;
				break;
			}
			if (!variable) {

				for (auto& pair: m_classType->object.privateMembers) {

					if (pair.second.type->type != type)
						continue;
					variable = &pair.second;
					break;
				}
			}
		}
		else {
			for (auto& pair: m_context.m_baseFrame->m_values) {

				if (pair.second.type->type != type)
					continue;
				variable = &pair.second;
				break;
			}
		}

		/* Write function signature */
		if (m_implementation)
			m_stream <<
				"$" << SerialName(m_classType->name) <<
				"____" << implHashString << "__" <<
				node.m_name << "__" <<
				variable->overloadID;

		else m_stream <<
			"$" << SerialName(name) << "____" <<
			variable->overloadID;

		m_stream << " :: ";
		writeFunctionSignature(type, m_classType);
		m_stream << " {\n";

		/* Abstract function */
		if (variable->flagAbstract) {

			m_stream <<
				"ret *ptr (*ptr (%this + 16) + " <<
				type->function.vtablePosition <<
				") (";

			std::size_t count = 0;
			if (returnType->base == BaseType::Any) {

				count++;
				m_stream << "%__return";
			}
			if (m_classType) {

				if (count++) m_stream << ", ";
				m_stream << "%this";
			}
			for (const auto& parameter: type->function.parameters) {

				if (count++) m_stream << ", ";
				m_stream << "%" << parameter.first;

				if (parameter.second.type->type->base == BaseType::Any)
					m_stream << "__param";
			}
			m_stream <<
				")" << baseTypeReturn(returnType->base) <<
				"\n}\n";

			/* Default implementation */
			m_stream <<
				"$" << SerialName(name) << "____" <<
				variable->overloadID << "impl :: ";

			writeFunctionSignature(type, m_classType);
			m_stream << " {\n";
		}
		if (returnType->base != BaseType::Any)
			m_stream <<
				"var %__return = " <<
				baseType(returnType->base) <<
				" {}\n";

		/* Cast "this" to correct type */
		if (m_classType)
			m_stream <<
				"%this = $__std__cast(%this, " <<
				Hash(m_classType->name).getValue() <<
				"u32)ptr\n";

		/* Function parameters */
		pushScope();
		for (const auto& parameter: type->function.parameters) {

			Type *paramType = parameter.second.type->type;

			if (paramType->base == BaseType::Any) {

				m_stream << "var %" << parameter.first << " = [16] {}\n";
				m_stream <<
					"*u64 (%" << parameter.first << " + 0) = " <<
					"*u64 (%" << parameter.first << "__param + 0)\n";
				m_stream <<
					"*u64 (%" << parameter.first << " + 8) = " <<
					"*u64 (%" << parameter.first << "__param + 8)\n";
			}

			/* Reference countable type */
			if (paramType->base != BaseType::Object &&
			    paramType->base != BaseType::Any)
				continue;

			std::pair<Name, BaseType> rc = {
				parameter.first,
				paramType->base
			};
			m_rcScope[0].emplace_back(std::move(rc));

			if (paramType->base == BaseType::Object)
				m_stream <<
					"$__std__incref(%" <<
					parameter.first <<
					")ptr\n";
			else
				m_stream <<
					"$__std__increfAny(0ptr, %" <<
					parameter.first <<
					")ptr\n";
		}

		/* Add "this" to reference countables */
		if (m_classType) {

			m_stream << "$__std__incref(%this)ptr\n";

			std::pair<Name, BaseType> rc = {
				"this", BaseType::Object
			};
			m_rcScope[0].emplace_back(std::move(rc));
		}
		m_noPushBlockScope = true;

		/* Initializer */
		if (m_classType && node.m_name == "init" && node.m_useDefault) {
			for (const auto& parameter: type->function.parameters) {

				Type *paramType = parameter.second.type->type;

				/* Get field for parameter */
				Result<VariableInfo *> result =
					m_classType->object.publicMembers.get(
						parameter.first
					);
				if (!result.okay ||
				    result.value->type->type->base ==
				    BaseType::Function) {

					result = m_classType->object.privateMembers.get(
						parameter.first
					);
					if (!result.okay ||
					    result.value->type->type->base ==
					    BaseType::Function)
						continue;
				}
				if (result.value->type->equalTo(
					*parameter.second.type
				    ) != TypeEquality::Equivalent)
					continue;
				VariableInfo *variable = result.value;

				/* Assign field */
				if (paramType->base == BaseType::Any)
					m_stream <<
						"*u64 (%this + " << variable->position <<
						"ptr) = *u64 (%" << parameter.first <<
						" + 0)\n*u64 (%this + " <<
						(variable->position + 8) <<
						"ptr) = *u64 (%" << parameter.first <<
						" + 8)\n$__std__increfAny(0ptr, (%this + " <<
						variable->position << "))ptr\n";

				else if (paramType->base == BaseType::Object)
					m_stream <<
						"*ptr (%this + " << variable->position <<
						"ptr) = $__std__incref(%" << parameter.first <<
						")ptr\n";

				else m_stream <<
					"*" << baseType(paramType->base) << " (%this + " <<
					variable->position << "ptr) = %" << parameter.first <<
					'\n';
			}
		}
		return Result<None>();
	}

	/* Visit "func" statement with expression body */
	Result<None> Compiler::visitFuncExprStatement(FuncExprStatementNode& node) {

		BEA_OK(None, visitFuncStatement(node));
		m_useStream = true;

		writeReturnValue(*node.m_bodyNode);
		m_rcScope.clear();

		m_useStream = false;
		m_stream << "\n}\n";

		return Result<None>();
	}

	/* Visit "func" statement with block body */
	Result<None> Compiler::visitFuncBlockStatement(FuncBlockStatementNode& node) {

		if (node.m_hollow)
			return Result<None>();

		BEA_OK(None, visitFuncStatement(node));
		m_useStream = true;

		BlockNode *blockNode = static_cast<BlockNode *>(
			node.m_bodyNode.get()
		);
		bool lastIsReturn = false;
		for (std::unique_ptr<Node>& statement: blockNode->m_nodes) {

			lastIsReturn = false;
			if (dynamic_cast<ReturnStatementNode *>(statement.get()))
				lastIsReturn = true;

			bool deleteRef = false;
			bool deleteAnyRef = false;

			/* Figure out if we need to try delete an object */
			if (statement->m_atom.flagExpression) {

				Type *type = statement->m_atom.type->type;
				if (type->base == BaseType::Object)
					deleteRef = true;
				else if (type->base == BaseType::Any)
					deleteAnyRef = true;
			}
			if (deleteRef) m_stream << "$__std__delete(";
			if (deleteAnyRef) m_stream << "$__std__deleteAny(";

			BEA_OK(None, visit(*statement));

			if (deleteRef) m_stream << ")u64\n";
			if (deleteAnyRef) m_stream << ")u64\n";

			m_stream << '\n';
		}

		/* Write end of scope */
		if (!lastIsReturn) {

			writeScopeEnd(0);
			m_stream << "ret %__return\n}\n";
		}
		else m_stream << "}\n";

		m_rcScope.clear();
		m_useStream = false;

		return Result<None>();
	}

	/* Visit class field statement */
	Result<None> Compiler::visitClassFieldStatement(ClassFieldStatementNode& node) {

		if (!node.m_valueNode) return Result<None>();

		VariableInfo *variable = nullptr;
		Result<VariableInfo *> result =
			m_classType->object.publicMembers.get(
				node.m_name
			);
		if (!result.okay)
			variable = m_classType->object.privateMembers.get(
				node.m_name
			).value;
		else
			variable = result.value;

		Type *type = variable->type->type;

		/* Assign member */
		m_ctorStream <<
			"*" << baseType(type->base) << " (%this + " <<
			variable->position << "ptr) = ";

		m_useCtorStream = true;
		BEA_OK(None, visit(*node.m_valueNode));
		m_useCtorStream = false;

		m_ctorStream << '\n';
		return Result<None>();
	}

	/* Visit "impl" statement */
	Result<None> Compiler::visitImplStatement(ImplStatementNode& node) {

		m_implementation = node.m_atom.operationImpl;

		m_noPushBlockScope = true;
		BEA_OK(None, visit(*node.m_bodyNode));

		m_implementation = nullptr;

		return Result<None>();
	}

	/* Visit "class" statement */
	Result<None> Compiler::visitClassStatement(ClassStatementNode& node) {

		Type *type = m_context.getType(node.m_name).value;
		type->getSize();
		type->getVtableSize();

		std::size_t interfaceCount =
			1 + type->object.implementations.size();

		std::size_t abstractCount = 0;
		for (const auto& pair: type->object.publicMembers) {

			if (pair.second.flagAbstract)
				abstractCount++;
		}
		for (const auto& pair: type->object.privateMembers) {

			if (pair.second.flagAbstract)
				abstractCount++;
		}

		/* Class and interface global variables */
		m_stream <<
			"$" << SerialName(node.m_name) << "____class :: [" <<
			(32 + interfaceCount * 8) << "] {}\n";
		m_stream <<
			"$" << SerialName(node.m_name) << "____if0 :: [" <<
			type->getVtableSize() << "] {}\n";

		std::size_t i = 1;
		for (const auto& implementation: type->object.implementations) {

			Type *implType = implementation.type->type;
			m_stream <<
				"$" << SerialName(node.m_name) << "____if" <<
				i << " :: [" << implType->getVtableSize() <<
				"] {}\n";
			i++;
		}

		/* Construct class information */
		m_initStream <<
			"*u64 ($" << SerialName(node.m_name) << "____class + 0) = " <<
			interfaceCount << "u64\n";

		m_initStream <<
			"*u64 ($" << SerialName(node.m_name) << "____class + 8) = " <<
			type->getSize() << "u64\n";

		m_initStream <<
			"*ptr ($" << SerialName(node.m_name) << "____class + 16) = $" <<
			SerialName(node.m_name) << "____dtor\n";

		for (std::size_t i = 0; i < interfaceCount; i++)
			m_initStream << "*ptr ($" << SerialName(node.m_name) <<
			"____class + " << (32 + i * 8) << ") = $" <<
			SerialName(node.m_name) << "____if" << i << "\n";

		/* Construct self vtable */
		m_initStream <<
			"*u32 ($" << SerialName(node.m_name) << "____if0 + 0) = " <<
			Hash(node.m_name).getValue() << "u32\n";

		for (const auto& pair: type->object.publicMembers) {

			if (!pair.second.flagAbstract)
				continue;
			Type *fType = pair.second.type->type;

			m_initStream <<
				"*ptr ($" << SerialName(node.m_name) << "____if0 + " <<
				fType->function.vtablePosition << ") = $" <<
				SerialName(node.m_name) << "__" << pair.first <<
				"____" << pair.second.overloadID << "impl\n";
		}
		for (const auto& pair: type->object.privateMembers) {

			if (!pair.second.flagAbstract)
				continue;
			Type *fType = pair.second.type->type;

			m_initStream <<
				"*ptr ($" << SerialName(node.m_name) << "____if0 + " <<
				fType->function.vtablePosition << ") = $" <<
				SerialName(node.m_name) << "__" << pair.first <<
				"____" << pair.second.overloadID << "impl\n";
		}

		/* Construct other vtables */
		i = 1;
		for (auto& implementation: type->object.implementations) {

			Type *implType = implementation.type->type;

			Hash implHash(*implementation.type);

			std::uint32_t implHashValue = implHash.getValue();
			std::string implHashString = implHash.toString();

			m_initStream <<
				"*u32 ($" << SerialName(node.m_name) << "____if" <<
				i << " + 0) = " << implHashValue << "u32\n";

			m_initStream <<
				"*u64 ($" << SerialName(node.m_name) << "____if" <<
				i << " + 4) = " << implementation.position << "u64\n";

			/* Set members */
			for (const auto& pair: implType->object.publicMembers) {

				if (!pair.second.flagAbstract)
					continue;
				Type *fType = pair.second.type->type;

				const VariableInfo *variable = nullptr;
				for (const auto& member: implementation.members) {

					if (member.first == pair.first &&
					    member.second.overloadID ==
					    pair.second.overloadID) {

						variable = &pair.second;
						break;
					}
				}
				m_initStream <<
					"*ptr ($" << SerialName(node.m_name) <<
					"____if" << i << " + " <<
					fType->function.vtablePosition <<
					") = $";

				if (!variable) /* Use default implementation */
					m_initStream <<
						SerialName(implType->name) <<
						"__" << pair.first << "____" <<
						pair.second.overloadID << "impl";

				else /* Use provided implementation */
					m_initStream <<
						SerialName(node.m_name) <<
						"____" << implHashString << "__" <<
						pair.first << "__" <<
						pair.second.overloadID;

				m_initStream << '\n';
			}
			for (const auto& pair: implType->object.privateMembers) {

				if (!pair.second.flagAbstract)
					continue;
				Type *fType = pair.second.type->type;

				const VariableInfo *variable = nullptr;
				for (const auto& member: implementation.members) {

					if (member.first == pair.first &&
					    member.second.overloadID ==
					    pair.second.overloadID) {

						variable = &pair.second;
						break;
					}
				}
				m_initStream <<
					"*ptr ($" << SerialName(node.m_name) <<
					"____if" << i << " + " <<
					fType->function.vtablePosition <<
					") = $";

				if (!variable) /* Use default implementation */
					m_initStream << SerialName(implType->name) <<
					"__" << pair.first << "__" <<
					pair.second.overloadID << "impl";

				else /* Use provided implementation */
					m_initStream << SerialName(node.m_name) <<
					"____" << implHashString << "__" <<
					pair.first << "__" << pair.second.overloadID;

				m_initStream << '\n';
			}
			i++;
		}
		m_ctorStream.str("");

		m_classType = type;

		m_noPushBlockScope = true;
		BEA_OK(None, visit(*node.m_bodyNode));

		m_classType = nullptr;

		/* Write class base constructor */
		m_stream << "$" << SerialName(node.m_name) <<
			"____ctor :: proc(%this ptr, %root ptr, %vtable ptr)u64 {\n";

		m_stream << "*ptr (%this + 0) = %root\n";
		m_stream <<
			"*ptr (%this + 8) = $" << SerialName(node.m_name) <<
			"____class\n";
		m_stream << "*ptr (%this + 16) = %vtable\n";

		i = 1;
		for (const auto& implementation: type->object.implementations) {

			Type *implType = implementation.type->type;

			m_stream <<
				"$" << SerialName(implType->name) <<
				"____ctor(%this + " << implementation.position <<
				", %root, $" << SerialName(node.m_name) <<
				"____if" << i << ")u64\n";
			i++;
		}
		m_stream << m_ctorStream.str() << "}\n";

		/* Write class destructor */
		m_stream << "$" << SerialName(node.m_name) <<
			"____dtor :: proc(%this ptr)u64 {\n";

		for (const auto& implementation: type->object.implementations) {

			Type *implType = implementation.type->type;

			m_stream <<
				"$" << SerialName(implType->name) <<
				"____dtor(%this + " << implementation.position <<
				")u64\n";
		}

		for (const auto& pair: type->object.publicMembers) {

			Type *memberType = pair.second.type->type;
			if (memberType->base == BaseType::Object)
				m_stream <<
					"$__std__decref(*ptr (%this + " <<
					pair.second.position << "))u64\n";
			else if (memberType->base == BaseType::Any)
				m_stream <<
					"$__std__decrefAny(%this + " <<
					pair.second.position << ")u64\n";
		}
		for (const auto& pair: type->object.privateMembers) {

			Type *memberType = pair.second.type->type;
			if (memberType->base == BaseType::Object)
				m_stream <<
					"$__std__decref(*ptr (%this + " <<
					pair.second.position << "))u64\n";
			else if (memberType->base == BaseType::Any)
				m_stream <<
					"$__std__decrefAny(%this + " <<
					pair.second.position << ")u64\n";
		}
		m_stream << "}\n";

		/* Write *new functions */
		for (const auto& pair: type->object.publicMembers) {

			Type *fType = pair.second.type->type;
			if (pair.first != "init" || fType->base != BaseType::Function)
				continue;
			m_stream <<
				"$" << SerialName(node.m_name) << "____new" <<
				pair.second.overloadID << " :: ";

			writeFunctionSignature(fType, nullptr, false);
			m_stream << " ptr {\n";

			/* Allocate object and call constructors */
			m_stream <<
				"var %object = ptr $__std__allocate($" <<
				SerialName(node.m_name) << "____class)ptr\n";
			m_stream <<
				"$" << SerialName(node.m_name) <<
				"____ctor(%object, %object, $" <<
				SerialName(node.m_name) <<
				"____if0)u64\n";
			m_stream <<
				"$__std__incref(%object)ptr\n" <<
				"$" << SerialName(node.m_name) <<
				"__init____" << pair.second.overloadID << "(%object";

			std::size_t count = 1;
			for (const auto& parameter: fType->function.parameters) {

				Type *paramType = parameter.second.type->type;

				if (count++) m_stream << ", ";
				m_stream << "%" << parameter.first;

				if (paramType->base == BaseType::Any)
					m_stream << "__param";
			}
			m_stream << ")u64\n$__std__decrefSafe(%object)u64\n";

			m_stream << "ret %object\n}\n";
		}
		return Result<None>();
	}

	/* Get scope stream */
	std::ostream& Compiler::scopeStream() {

		if (m_useStream)
			return m_stream;
		else if (m_useCtorStream)
			return m_ctorStream;
		else if (m_useInitStream)
			return m_initStream;
		return m_dummyStream;
	}

	/* Push scope */
	void Compiler::pushScope() {

		std::vector<std::pair<Name, BaseType>> scope;
		m_rcScope.push_back(std::move(scope));
	}

	/* Pop scope */
	void Compiler::popScope() {

		m_rcScope.pop_back();
	}

	/* Get string type name for base type */
	const char *Compiler::baseType(BaseType type) {

		switch (type) {
			using enum BaseType;
			case U8: return "u8";
			case I8: return "i8";
			case U16: return "u16";
			case I16: return "i16";
			case U32: return "u32";
			case I32: return "i32";
			case U64: return "u64";
			case I64: return "i64";
			case F32: return "f32";
			case F64: return "f64";
		}
		return "ptr";
	}

	/* Get string type name for base return type */
	const char *Compiler::baseTypeReturn(BaseType type) {

		if (type == BaseType::Any)
			return "u64";
		return baseType(type);
	}

	/* Write function signature */
	void Compiler::writeFunctionSignature(
		const Type *type,
		const Type *classType,
		bool writeReturnType
	) {
		Type *returnType = type->function.returnType->type;
		m_stream << "proc(";

		std::size_t count = 0;
		if (returnType->base == BaseType::Any) {

			count++;
			m_stream << "%__return ptr";
		}
		if (classType) {

			if (count++) m_stream << ", ";
			m_stream << "%this ptr";
		}
		for (const auto& parameter: type->function.parameters) {

			if (count++) m_stream << ", ";
			m_stream << "%" << parameter.first;

			if (parameter.second.type->type->base == BaseType::Any)
				m_stream << "__param";

			m_stream << " " << baseType(parameter.second.type->type->base);
		}
		m_stream << ")";
		if (writeReturnType)
			m_stream <<
				" " <<
				baseTypeReturn(returnType->base);
	}

	/* Write end of scope */
	void Compiler::writeScopeEnd(std::size_t toLevel) {

		for (std::size_t i = m_rcScope.size(); i > toLevel; i--) {

			for (const auto& pair: m_rcScope[i-1]) {

				if (pair.second == BaseType::Any)
					m_stream <<
						"$__std__decrefAny(%" <<
						pair.first << ")u64\n";
				else
					m_stream <<
						"$__std__decref(%" <<
						pair.first << ")u64\n";
			}
		}
	}

	/* Write return value */
	Result<None> Compiler::writeReturnValue(Node& node) {

		Type *otherType = node.m_atom.type->type;
		if (m_returnType->base == BaseType::Any) {

			if (otherType->base == BaseType::Any) {

				m_stream << "$memcpy(%__return, ";
				BEA_OK(None, visit(node));
				m_stream << ", 16u64)ptr\n";
			}
			else {
				m_stream <<
					"$__std__setAny(%__return, " <<
					(int)otherType->base << "u64, u64 ";
				BEA_OK(None, visit(node));
				m_stream << ")u64\n";
			}
		}
		else {
			if (otherType->base == BaseType::Any) {

				m_stream << "%__return = $__std__getAny(";
				BEA_OK(None, visit(node));
				m_stream << ", " << (int)m_returnType->base << "u64)" <<
				baseType(m_returnType->base) << "\n";
			}
			else {
				m_stream << "%__return = ";
				BEA_OK(None, visit(node));
				m_stream << "\n";
			}
		}

		/* Protect return value */
		if (m_returnType->base == BaseType::Object)
			m_stream << "$__std__incref(%__return)ptr\n";
		else if (m_returnType->base == BaseType::Any)
			m_stream << "$__std__increfAny(0ptr, %__return)ptr\n";

		writeScopeEnd(0);

		if (m_returnType->base == BaseType::Object)
			m_stream << "$__std__decrefSafe(%__return)u64\n";
		else if (m_returnType->base == BaseType::Any)
			m_stream << "$__std__decrefSafeAny(%__return)u64\n";

		m_stream << "ret %__return";
		return Result<None>();
	}

	/* Serialize name */
	std::ostream& operator<<(
		std::ostream& stream,
		Compiler::SerialName&& serialName
	) {
		const Name& name = serialName.name;

		std::size_t count = 0;
		for (const std::string& component: name) {

			if (count++) stream << "__";
			stream << component;
		}
		return stream;
	}
}
