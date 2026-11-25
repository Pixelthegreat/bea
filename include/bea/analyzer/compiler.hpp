/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/context.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * Compiler
	 */
	class Compiler : public NodeAnalyzerVisitor {
	public:
		/*
		 * Temporary container for convenient
		 * name serialization
		 */
		struct SerialName {
			const Name& name;

			SerialName(const Name& name) : name(name) {}
		};

		/*
		 * Compiler
		 */
		Compiler(
			Context& context,
			std::ostream& stream
		)
			: NodeAnalyzerVisitor(context),
			  m_stream(stream) {}

		virtual Result<None> analyze(Node& node) override;

		virtual Result<None> visitBlock(BlockNode& node) override;

		virtual Result<None> visitInteger(IntegerNode& node) override;
		virtual Result<None> visitFloat(FloatNode& node) override;
		virtual Result<None> visitString(StringNode& node) override;

		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;
		virtual Result<None> visitAssignment(AssignmentNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;
		virtual Result<None> visitNew(NewNode& node) override;
		virtual Result<None> visitNamedAccess(NamedAccessNode& node) override;

		virtual Result<None> visitReturnStatement(ReturnStatementNode& node) override;

		virtual Result<None> visitLetStatement(LetStatementNode& node) override;

		Result<None> visitFuncStatement(FuncStatementNode& node);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
		virtual Result<None> visitClassFieldStatement(ClassFieldStatementNode& node) override;
		virtual Result<None> visitImplStatement(ImplStatementNode& node) override;
		virtual Result<None> visitClassStatement(ClassStatementNode& node) override;
	private:
		std::ostream& m_stream;
		std::ostringstream m_initStream;
		std::ostringstream m_ctorStream;

		std::ostringstream m_dummyStream;
		std::ostringstream m_globalStream;

		bool m_useStream = false;
		bool m_useInitStream = false;
		bool m_useCtorStream = false;

		Type *m_classType = nullptr;
		ObjectImplementation *m_implementation = nullptr;

		Type *m_returnType = nullptr;

		bool m_noPushBlockScope = false;

		std::size_t m_stringID = 0;

		std::vector<std::vector<std::pair<Name, BaseType>>> m_rcScope;

		/*
		 * Get stream appropriate for scope
		 */
		std::ostream& scopeStream();

		/*
		 * Push scope
		 */
		void pushScope();

		/*
		 * Pop scope
		 */
		void popScope();

		/*
		 * Get base type names
		 */
		const char *baseType(BaseType type);
		const char *baseTypeReturn(BaseType type);

		/*
		 * Write function signature
		 */
		void writeFunctionSignature(
			const Type *type,
			const Type *classType,
			bool writeReturnType = true
		);

		/*
		 * Write end of scope
		 */
		void writeScopeEnd(std::size_t toLevel);

		/*
		 * Write return value
		 */
		Result<None> writeReturnValue(Node& node);
	};

	std::ostream& operator<<(
		std::ostream& stream,
		Compiler::SerialName&& serialName
	);
}
