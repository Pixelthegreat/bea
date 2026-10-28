#pragma once

#include <unordered_map>
#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	/*
	 * Stack frame
	 */
	class Frame {
	public:
		/*
		 * Iterator
		 */
		const std::pair<std::string, VariableInfo> *data() const { return m_values.data(); }
		const std::size_t size() const { return m_values.size(); }
		const bool empty() const { return m_values.empty(); }
		const std::pair<std::string, VariableInfo> *begin() const { return m_values.begin(); }
		const std::pair<std::string, VariableInfo> *end() const { return m_values.end(); }

		/*
		 * Get items
		 */
		Result<VariableInfo *> get(const std::string& key);
		Result<VariableInfo *> get(std::string&& key);

		VariableInfo& operator[](const std::string& key);
		VariableInfo& operator[](std::string&& key);

		VariableInfo& newEntry(const std::string& key);
		VariableInfo& newEntry(std::string&& key);
	private:
		OrderedMap<std::string, VariableInfo> m_values;
	};

	/*
	 * Analysis context
	 */
	class Context {
	public:
		Context() {

			m_currentFrame = m_baseFrame = pushFrame();
			initTypes();
		}

		std::unordered_map<std::string, std::unique_ptr<Type>> m_types;
		std::vector<std::unique_ptr<Frame>> m_frames;

		const Type *m_intType;
		const Type *m_floatType;
		const Type *m_booleanType;
		const Type *m_stringType;

		Frame *m_currentFrame;
		Frame *m_baseFrame;

		/*
		 * Get type
		 */
		Result<Type *> getType(const std::string& name);

		/*
		 * Register type
		 *
		 * Fails if it is already registered.
		 */
		Result<None> registerType(
			const std::string& name,
			std::unique_ptr<Type> type
		);

		/*
		 * Push and pop stack frames
		 */
		Frame *pushFrame();
		void popFrame();
	private:
		/*
		 * Initialize types
		 */
		void initTypes();
	};
}
