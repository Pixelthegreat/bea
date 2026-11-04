#pragma once

#include <bea/core.hpp>
#include <bea/serial.hpp>
#include <bea/node.hpp>
#include <bea/node/analyzer.hpp>

namespace bea {

	/*
	 * This analyzer declares global variables,
	 * functions and types and serializes them.
	 */
	class GlobalResolver : public NodeAnalyzerVisitor {
	public:
		/*
		 * Reference
		 */
		enum class ReferenceType {
			Type = 0,
			Entity,

			Count,
		};
		struct Reference {
			ReferenceType type;
			const Name& name;
			Node& node;
		};

		/*
		 * Analyzer implementation
		 */
		GlobalResolver(
			Context& context,
			GRCodeSerializer& serializer
		)
			: NodeAnalyzerVisitor(context),
			  m_serializer(serializer) {}

		virtual Result<None> analyze(Node& node) override;

		virtual Result<None> visitBlock(BlockNode& node) override;

		virtual Result<None> visitType(TypeNode& node) override;
		virtual Result<None> visitVariableAccess(VariableAccessNode& node) override;

		virtual Result<None> visitIfStatement(IfStatementNode& node) override;
		virtual Result<None> visitCaseStatement(CaseStatementNode& node) override;
		virtual Result<None> visitCaseLetStatement(CaseLetStatementNode& node) override;
		virtual Result<None> visitWhichStatement(WhichStatementNode& node) override;
		virtual Result<None> visitForStatement(ForStatementNode& node) override;
		virtual Result<None> visitWhileStatement(WhileStatementNode& node) override;

		virtual Result<None> visitUseStatement(UseStatementNode& node) override;
		virtual Result<None> visitLetStatement(LetStatementNode& node) override;

		Result<None> visitFuncStatement(
			FuncStatementNode& node,
			Node& bodyNode
		);
		virtual Result<None> visitFuncExprStatement(FuncExprStatementNode& node) override;
		virtual Result<None> visitFuncBlockStatement(FuncBlockStatementNode& node) override;

		virtual Result<None> visitClassFieldStatement(ClassFieldStatementNode& node) override;
		virtual Result<None> visitImplStatement(ImplStatementNode& node) override;
		virtual Result<None> visitClassStatement(ClassStatementNode& node) override;
	private:
		GRCodeSerializer& m_serializer;
		OrderedMap<std::string, VariableInfo> m_implMembers;
		std::unordered_map<Name, std::unique_ptr<Type>> m_implTypes;
		std::vector<Reference> m_externalReferences;

		/*
		 * Create type identifier for type node
		 */
		Result<std::unique_ptr<TypeID>> nodeToTypeID(Node *node);

		/*
		 * Register external references
		 */
		void registerTypeReference(
			const Name& name, Node& node
		);
		void registerEntityReference(
			const Name& name, Node& node
		);
	};

	std::ostream& operator<<(
		std::ostream& stream,
		const GlobalResolver::Reference& reference
	);
}
