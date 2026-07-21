#include <bea/core.hpp>
#include <bea/node/visitor.hpp>

namespace bea {

	Result<None> NodeVisitor::visit(Node& node) {

		return node.visit(*this);
	}

#define DEFAULT_IMPL(p_name) Result<None> NodeVisitor::visit##p_name(p_name##Node& node) {\
		return Result<None>();\
	}

	DEFAULT_IMPL(Block);
	DEFAULT_IMPL(Integer);
	DEFAULT_IMPL(Float);
	DEFAULT_IMPL(String);
	DEFAULT_IMPL(Type);
	DEFAULT_IMPL(VariableAccess);
	DEFAULT_IMPL(UnaryOperation);
	DEFAULT_IMPL(Assignment);
	DEFAULT_IMPL(BinaryOperation);
	DEFAULT_IMPL(CarryResult);
	DEFAULT_IMPL(New);
	DEFAULT_IMPL(Cast);
	DEFAULT_IMPL(List);
	DEFAULT_IMPL(Call);
	DEFAULT_IMPL(NamedAccess);
	DEFAULT_IMPL(IndexedAccess);
	DEFAULT_IMPL(Range);
	DEFAULT_IMPL(IfStatement);
	DEFAULT_IMPL(CaseStatement);
	DEFAULT_IMPL(CaseLetStatement);
	DEFAULT_IMPL(WhichStatement);
	DEFAULT_IMPL(ForStatement);
	DEFAULT_IMPL(WhileStatement);
}
