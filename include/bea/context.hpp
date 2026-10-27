#pragma once

#include <unordered_map>
#include <bea/core.hpp>
#include <bea/type.hpp>

namespace bea {

	/*
	 * Analysis context
	 */
	class Context {
	public:
		Context() {

			initTypes();
		}

		std::unordered_map<std::string, std::unique_ptr<Type>> m_types;

		const Type *m_intType;
		const Type *m_floatType;
		const Type *m_booleanType;
		const Type *m_stringType;

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
	private:
		/*
		 * Initialize types
		 */
		void initTypes();
	};
}
