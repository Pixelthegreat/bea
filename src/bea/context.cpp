#include <bea/core.hpp>
#include <bea/context.hpp>

namespace bea {

	/* Get item from frame */
	Result<VariableInfo *> Frame::get(const Name& key, bool parent) {

		Result<VariableInfo *> result = m_values.get(key);
		if (!result.okay && parent && m_parent)
			return m_parent->get(key, parent);
		return result;
	}
	Result<VariableInfo *> Frame::get(Name&& key, bool parent) {

		Result<VariableInfo *> result = m_values.get((const Name&)key);
		if (!result.okay && parent && m_parent)
			return m_parent->get(std::move(key), parent);
		return result;
	}

	/* Get or create item in frame */
	VariableInfo& Frame::operator[](const Name& key) {

		return m_values[key];
	}
	VariableInfo& Frame::operator[](Name&& key) {

		return m_values[std::move(key)];
	}

	/* Create item in frame */
	VariableInfo& Frame::newEntry(const Name& key) {

		return m_values.newEntry(key);
	}
	VariableInfo& Frame::newEntry(Name&& key) {

		return m_values.newEntry(std::move(key));
	}

	/* Get overload count name */
	std::size_t& Frame::getOverloadCount(
		const Name& name
	) {
		return m_overloads[std::hash<Name>{}(name)];
	}

	/* Get type */
	Result<Type *> Frame::getType(const Name& name, bool parent) {

		Type *type = nullptr;

		try {
			type = m_types.at(name).get();
		}
		catch (std::out_of_range) {}

		if (!type) {

			if (parent && m_parent)
				return m_parent->getType(name, parent);
			return Result<Type *>::fail();
		}
		return Result<Type *>(type);
	}

	/* Register type */
	Result<None> Frame::registerType(
		const Name& name,
		std::unique_ptr<Type> type
	) {
		std::unique_ptr<Type>& slot = m_types[name];

		if (slot.get()) return Result<None>::fail();

		slot = std::move(type);
		return Result<None>();
	}

	/* Push stack frame */
	Frame *Context::pushFrame() {

		m_frames.emplace_back(std::make_unique<Frame>(m_currentFrame));
		m_currentFrame = m_frames[m_frames.size()-1].get();

		return m_currentFrame;
	}

	/* Pop stack frame */
	void Context::popFrame() {

		m_frames.pop_back();
		if (!m_frames.empty())
			m_currentFrame = m_frames[m_frames.size()-1].get();
		else
			m_currentFrame = nullptr;
	}

	/* Get type from current frame */
	Result<Type *> Context::getType(const Name& name, bool parent) {

		return m_currentFrame->getType(name, parent);
	}

	/* Register type in current frame */
	Result<None> Context::registerType(
		const Name& name,
		std::unique_ptr<Type> type
	) {
		return m_currentFrame->registerType(name, std::move(type));
	}

	/* Resolve aliases */
	Result<None> Context::resolveAliases() {

		for (Reference& reference: m_externalReferences) {

			if (reference.dealiased)
				continue;

			Name name = reference.name;

			auto it = m_aliases.end();
			do {
				it = m_aliases.find(name);
				name.components.pop_back();

			} while (name.components.size() && it == m_aliases.end());

			if (it == m_aliases.end())
				continue;
			const Name& realName = (*it).second;

			std::cout << reference.name << " -> ";

			reference.name.components.erase(
				reference.name.components.begin(),
				reference.name.components.begin() +
				name.components.size() + 1
			);
			for (std::size_t i = 0; i < realName.components.size(); i++)
				reference.name.components.insert(
					reference.name.components.begin()+i,
					realName.components[i]
				);
			std::cout << reference.name << std::endl;

			reference.dealiased = true;
		}
		return Result<None>();
	}

	/* Resolve references */
	Result<None> Context::resolveReferences() {

		for (Reference& reference: m_externalReferences) {

			if (reference.resolved)
				continue;

			/* Type reference */
			if (reference.type == ReferenceType::Type) {

				Result<Type *> result =
					m_baseFrame->getType(reference.name);
				if (!result.okay) continue;

				if (reference.pType)
					*reference.pType = result.value;
			}

			/* Entity reference */
			else if (reference.type == ReferenceType::Entity) {

				Result<VariableInfo *> result =
					m_baseFrame->get(reference.name);
				if (!result.okay) continue;
			}
			reference.resolved = true;
		}
		return Result<None>();
	}

	/* Initialize base types */
	void Context::initTypes() {

		std::unique_ptr<Type> intType = std::make_unique<Type>(
			BaseType::Int, "Int"
		);
		m_intType = intType.get();

		registerType("Int", std::move(intType));

		/* Float */
		std::unique_ptr<Type> floatType = std::make_unique<Type>(
			BaseType::Float, "Float"
		);
		m_floatType = floatType.get();

		registerType("Float", std::move(floatType));

		/* Boolean */
		std::unique_ptr<Type> booleanType = std::make_unique<Type>(
			BaseType::Boolean, "Boolean"
		);
		m_booleanType = booleanType.get();

		registerType("Boolean", std::move(booleanType));

		/* String */
		std::unique_ptr<Type> stringType = std::make_unique<Type>("String");
		m_stringType = stringType.get();

		stringType->object = {
		};
		registerType("String", std::move(stringType));
	}

	/* Initialize aliases */
	void Context::initAliases() {

		if (m_sourcePath != "stdlib/src/iterator.bea") {

			m_aliases["Iterable"] = {"std", "iterator", "Iterable"};
			m_aliases["Iterator"] = {"std", "iterator", "Iterator"};
		}
		if (m_sourcePath != "stdlib/src/range.bea")
			m_aliases["Range"] = {"std", "range", "Range"};

		if (m_sourcePath != "stdlib/src/result.bea")
			m_aliases["Result"] = {"std", "result", "Result"};
	}

	/* Reference printer */
	std::ostream& operator<<(
		std::ostream& stream,
		const Context::Reference& reference
	) {
		stream << reference.node.m_position << ":" << reference.name << "(";
		switch (reference.type) {
			case Context::ReferenceType::Type:
				stream << "Type";
				break;
			case Context::ReferenceType::Entity:
				stream << "Entity";
		}
		stream << ")";

		return stream;
	}
}
