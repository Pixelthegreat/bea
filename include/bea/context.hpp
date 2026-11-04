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
		Frame(Frame *parent = nullptr)
			: m_parent(parent) {}

		/*
		 * Iterator
		 */
		std::pair<std::string, VariableInfo> *data() { return m_values.data(); }
		std::size_t size() const { return m_values.size(); }
		bool empty() const { return m_values.empty(); }
		std::pair<std::string, VariableInfo> *begin() { return m_values.begin(); }
		std::pair<std::string, VariableInfo> *end() { return m_values.end(); }
		const std::pair<std::string, VariableInfo> *data() const { return m_values.data(); }
		const std::pair<std::string, VariableInfo> *begin() const { return m_values.begin(); }
		const std::pair<std::string, VariableInfo> *end() const { return m_values.end(); }

		/*
		 * Get items
		 */
		Result<VariableInfo *> get(const std::string& key, bool parent = false);
		Result<VariableInfo *> get(std::string&& key, bool parent = false);

		VariableInfo& operator[](const std::string& key);
		VariableInfo& operator[](std::string&& key);

		VariableInfo& newEntry(const std::string& key);
		VariableInfo& newEntry(std::string&& key);

		/*
		 * Get overload count for name
		 */
		std::size_t& getOverloadCount(
			const Name& name
		);

		/*
		 * Get type
		 */
		Result<Type *> getType(const Name& name, bool parent = false);

		/*
		 * Register type
		 *
		 * Fails if it is already registered.
		 */
		Result<None> registerType(
			const Name& name,
			std::unique_ptr<Type> type
		);

		OrderedMap<std::string, VariableInfo> m_values;
		std::unordered_map<std::size_t, std::size_t> m_overloads;
		std::unordered_map<Name, std::unique_ptr<Type>> m_types;

		Frame *m_parent = nullptr;
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
		std::vector<std::unique_ptr<Frame>> m_frames;

		Type *m_intType = nullptr;
		Type *m_floatType = nullptr;
		Type *m_booleanType = nullptr;
		Type *m_stringType = nullptr;

		Frame *m_currentFrame = nullptr;
		Frame *m_baseFrame = nullptr;

		/*
		 * Push and pop stack frames
		 */
		Frame *pushFrame();
		void popFrame();

		/*
		 * Get type from current frame
		 */
		Result<Type *> getType(const Name& name, bool parent = false);

		/*
		 * Register type in current frame
		 *
		 * Fails if it is already registered.
		 */
		Result<None> registerType(
			const Name& name,
			std::unique_ptr<Type> type
		);
	private:
		/*
		 * Initialize types
		 */
		void initTypes();
	};
}
