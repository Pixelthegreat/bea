#include <bea/core.hpp>
#include <bea/context.hpp>

namespace bea {

	/*
	 * Initialize base types
	 */
	void Context::initTypes() {

		std::unique_ptr<Type> intType = std::make_unique<Type>(
			BaseType::Int, "Int"
		);
		m_intType = intType.get();

		m_types["Int"] = std::move(intType);

		/* Float */
		std::unique_ptr<Type> floatType = std::make_unique<Type>(
			BaseType::Float, "Float"
		);
		m_floatType = floatType.get();

		m_types["Float"] = std::move(floatType);

		/* Boolean */
		std::unique_ptr<Type> booleanType = std::make_unique<Type>(
			BaseType::Boolean, "Boolean"
		);
		m_booleanType = booleanType.get();

		booleanType->distinct = true;
		m_types["Boolean"] = std::move(booleanType);

		/* String */
		std::unique_ptr<Type> stringType = std::make_unique<Type>("String");
		m_stringType = stringType.get();

		stringType->object = {
		};
		m_types["String"] = std::move(stringType);
	}

	/* Get type */
	Result<Type *> Context::getType(const std::string& name) {

		Type *type = nullptr;

		try {
			type = m_types.at(name).get();
		}
		catch (std::out_of_range) {}

		if (!type) return Result<Type *>::fail();
		return Result<Type *>();
	}

	/* Register type */
	Result<None> Context::registerType(
		const std::string& name,
		std::unique_ptr<Type> type
	) {
		std::unique_ptr<Type>& slot = m_types[name];

		if (slot.get()) return Result<None>::fail();

		slot = std::move(type);
		return Result<None>();
	}
}
