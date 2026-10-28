#include <bea/core.hpp>
#include <bea/context.hpp>

namespace bea {

	/*
	 * Get item from frame
	 */
	Result<VariableInfo *> Frame::get(const std::string& key) {

		return m_values.get(key);
	}
	Result<VariableInfo *> Frame::get(std::string&& key) {

		return m_values.get(std::move(key));
	}

	/*
	 * Get or create item in frame
	 */
	VariableInfo& Frame::operator[](const std::string& key) {

		return m_values[key];
	}
	VariableInfo& Frame::operator[](std::string&& key) {

		return m_values[std::move(key)];
	}

	/*
	 * Create item in frame
	 */
	VariableInfo& Frame::newEntry(const std::string& key) {

		return m_values.newEntry(key);
	}
	VariableInfo& Frame::newEntry(std::string&& key) {

		return m_values.newEntry(std::move(key));
	}

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

	/*
	 * Push stack frame
	 */
	Frame *Context::pushFrame() {

		m_frames.emplace_back(std::make_unique<Frame>());
		m_currentFrame = m_frames[m_frames.size()-1].get();

		return m_currentFrame;
	}

	/*
	 * Pop stack frame
	 */
	void Context::popFrame() {

		m_frames.pop_back();
		if (!m_frames.empty())
			m_currentFrame = m_frames[m_frames.size()-1].get();
		else
			m_currentFrame = nullptr;
	}
}
