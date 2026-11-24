#pragma once

#include <unordered_map>
#include <bea/core.hpp>
#include <bea/type.hpp>
#include <bea/node.hpp>

namespace bea {

	/*
	 * Stack frame
	 */
	class Frame {
	public:
		Frame(Frame *parent = nullptr)
			: m_parent(parent) {}

		OrderedMap<Name, VariableInfo> m_values;
		std::unordered_map<std::size_t, std::size_t> m_overloads;
		std::unordered_map<Name, std::unique_ptr<Type>> m_types;

		Frame *m_parent = nullptr;

		/*
		 * Iterator
		 */
		std::pair<Name, VariableInfo> *data() { return m_values.data(); }
		std::size_t size() const { return m_values.size(); }
		bool empty() const { return m_values.empty(); }
		std::pair<Name, VariableInfo> *begin() { return m_values.begin(); }
		std::pair<Name, VariableInfo> *end() { return m_values.end(); }
		const std::pair<Name, VariableInfo> *data() const { return m_values.data(); }
		const std::pair<Name, VariableInfo> *begin() const { return m_values.begin(); }
		const std::pair<Name, VariableInfo> *end() const { return m_values.end(); }

		/*
		 * Get items
		 */
		Result<VariableInfo *> get(const Name& key, bool parent = false);
		Result<VariableInfo *> get(Name&& key, bool parent = false);
		Result<VariableInfo *> get(const Name& key, std::size_t overloadID);

		VariableInfo& operator[](const Name& key);
		VariableInfo& operator[](Name&& key);

		VariableInfo& newEntry(const Name& key);
		VariableInfo& newEntry(Name&& key);

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
	};

	/*
	 * Analysis context
	 */
	class Context {
	public:
		/*
		 * External reference record
		 */
		enum class ReferenceType {
			Type = 0,
			Entity,

			Count,
		};
		struct Reference {
			ReferenceType type;
			Name& name;
			Node& node;
			Position position;
			Type **pType = nullptr;
			bool dealiased = false;
			bool resolved = false;
		};

		/*
		 * Context
		 */
		Context(
			const std::string& sourcePath,
			const Name& moduleName
		)
			: m_sourcePath(sourcePath),
			  m_moduleName(moduleName) {

			m_currentFrame = m_baseFrame = pushFrame();
			initTypes();
			initAliases();
			initReferences();
			initConstants();
		}
		const std::string& m_sourcePath;
		const Name& m_moduleName;

		std::vector<std::unique_ptr<Frame>> m_frames;
		std::vector<std::unique_ptr<Frame>> m_deadFrames;

		Type *m_intType = nullptr;
		Type *m_floatType = nullptr;
		Type *m_booleanType = nullptr;

		Frame *m_currentFrame = nullptr;
		Frame *m_baseFrame = nullptr;

		std::vector<Reference> m_externalReferences;
		std::unordered_map<Name, Name> m_aliases;

		std::vector<std::vector<Name>> m_strayNames;
		std::vector<TypeID *> m_strayTypeIDs;

		/*
		 * Push and pop stack frames
		 */
		Frame *pushFrame();
		void popFrame();

		/*
		 * Clear dead stack frames
		 */
		void clearDeadFrames();

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

		/*
		 * Resolve aliases
		 */
		Result<None> resolveAliases();

		/*
		 * Resolve references
		 */
		Result<None> resolveReferences(bool internal = false);

		/*
		 * Resolve stray type identifiers
		 */
		void resolveStrayTypeIDs();

		/*
		 * Resolve module
		 */
		Result<None> resolveModule(
			const std::string& path,
			const Name& name,
			const std::string& sourcePath
		);
	private:
		/*
		 * Initialize types
		 */
		void initTypes();

		/*
		 * Initialize aliases
		 */
		void initAliases();

		/*
		 * Initialize proxy references
		 */
		void initReferences();

		/*
		 * Initialize constants
		 */
		void initConstants();
	};

	std::ostream& operator<<(
		std::ostream& stream,
		const Context::Reference& reference
	);
}
