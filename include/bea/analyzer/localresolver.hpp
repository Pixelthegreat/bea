#pragma once

#include <bea/core.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * This analyzer declares local variables,
	 * checks types and in general verifies that
	 * the code can be compiled.
	 */
	class LocalResolver : public NodeAnalyzerVisitor {
	public:
		LocalResolver(Context& context)
			: NodeAnalyzerVisitor(context) {}

		virtual Result<None> visitInteger(IntegerNode& node) override;
		virtual Result<None> visitFloat(FloatNode& node) override;
		virtual Result<None> visitString(StringNode& node) override;
		virtual Result<None> visitType(TypeNode& node) override;

		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;
		virtual Result<None> visitAssignment(AssignmentNode& node) override;
		virtual Result<None> visitBinaryOperation(BinaryOperationNode& node) override;

		virtual Result<None> visitLetStatement(LetStatementNode& node) override;

		Result<None> visitFuncStatement(FuncStatementNode& node);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;
	private:
		TypeID *m_returnType = nullptr;

		/*
		 * Get type for type identifier
		 */
		std::unique_ptr<TypeID> registerTypeID(
			std::unique_ptr<TypeID> typeID
		);

		/*
		 * Validate type identifier
		 */
		Result<None> validateTypeID(
			const TypeID& typeID,
			const Position& position
		);
	};
}
