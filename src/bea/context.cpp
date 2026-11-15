#include <fstream>
#include <bea/core.hpp>
#include <bea/serial.hpp>
#include <bea/context.hpp>

namespace bea {

	static IntegerNode dummyNode(0);

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

	/* Get item from frame with overload id */
	Result<VariableInfo *> Frame::get(const Name& key, std::size_t overloadID) {

		std::size_t hash = std::hash<Name>{}(key);

		for (auto *it = m_values.end(); it != m_values.begin(); it--) {

			auto *pair = it-1;

			if (std::hash<Name>{}(pair->first) == hash &&
			    pair->second.overloadID == overloadID)
				return Result<VariableInfo *>(&pair->second);
		}
		return Result<VariableInfo *>::fail();
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

		m_deadFrames.emplace_back(std::move(
			m_frames[m_frames.size()-1]
		));
		m_frames.pop_back();

		if (!m_frames.empty())
			m_currentFrame = m_frames[m_frames.size()-1].get();
		else
			m_currentFrame = nullptr;
	}

	/* Clear dead stack frames */
	void Context::clearDeadFrames() {

		m_deadFrames.clear();
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

			/* replace names */
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
			TypeNode *typeNode =
				dynamic_cast<TypeNode *>(
					&reference.node
				);
			if (typeNode) typeNode->m_name = reference.name;

			VariableAccessNode *varNode =
				dynamic_cast<VariableAccessNode *>(
					&reference.node
				);
			if (varNode) varNode->m_name = reference.name;

			reference.dealiased = true;
		}
		return Result<None>();
	}

	/* Resolve references */
	Result<None> Context::resolveReferences(bool internal) {

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

	/* Resolve module */
	Result<None> Context::resolveModule(
		const std::string& path,
		const Name& name,
		const std::string& sourcePath
	) {
		m_aliases.clear();

		std::ifstream stream(path, std::ios::binary);
		if (!stream.is_open())
			return Result<None>(
				std::format(
					"Can't open file '{}': {}",
					path, std::strerror(errno),
					name
				)
			);

		/* Deserialize file */
		GRCodeDeserializer deserializer(
			stream, name,
			sourcePath
		);
		BEA_OK(None, deserializer.readAll());

		stream.close();

		/* Move aliases */
		for (Pair<Name> pair: deserializer.m_aliases)
			m_aliases[std::move(pair[1])] =
				std::move(pair[0]);

		/* Move types */
		for (std::pair<Name, std::unique_ptr<Type>>& pair: deserializer.m_types)
			m_baseFrame->m_types[std::move(pair.first)] =
				std::move(pair.second);

		m_strayNames.emplace_back(
			deserializer.m_externalReferences.size()
		);
		std::vector<Name>& strayNames =
			m_strayNames[m_strayNames.size()-1];

		std::size_t i = 0;
		for (
			const std::tuple<GRCode, Name, Position>& tuple:
			deserializer.m_externalReferences
		) {
			ReferenceType type = ReferenceType::Type;
			if (std::get<0>(tuple) == GRCode::ExternalEntity)
				type = ReferenceType::Entity;

			strayNames[i] = std::move(std::get<1>(tuple));

			Reference reference = {
				.type = type,
				.name = strayNames[i],
				.node = dummyNode,
			};
			m_externalReferences.emplace_back(
				std::move(reference)
			);
			i++;
		}

		/* Move variables */
		for (std::pair<Name, VariableInfo>& pair: deserializer.m_values)
			m_currentFrame->newEntry(std::move(pair.first)) =
				std::move(pair.second);

		/* Record stray type IDs */
		for (TypeID *typeID: deserializer.m_typeIDs)
			m_strayTypeIDs.push_back(typeID);

		/* Resolve aliases and references */
		BEA_OK(None, resolveAliases());
		BEA_OK(None, resolveReferences());

		return Result<None>();
	}

	/* Initialize base types */
	void Context::initTypes() {

		std::unique_ptr<Type> intType = std::make_unique<Type>(
			BaseType::Int, "Int"
		);
		m_intType = intType.get();
		m_intType->external = false;

		registerType("Int", std::move(intType));

		/* Float */
		std::unique_ptr<Type> floatType = std::make_unique<Type>(
			BaseType::Float, "Float"
		);
		m_floatType = floatType.get();
		m_floatType->external = false;

		registerType("Float", std::move(floatType));

		/* Boolean */
		std::unique_ptr<Type> booleanType = std::make_unique<Type>(
			BaseType::Boolean, "Boolean"
		);
		m_booleanType = booleanType.get();
		m_booleanType->external = false;

		registerType("Boolean", std::move(booleanType));

		/* String */
		std::unique_ptr<Type> stringType = std::make_unique<Type>("String");
		m_stringType = stringType.get();
		m_stringType->external = false;

		stringType->object = {
		};
		registerType("String", std::move(stringType));
	}

	/* Initialize aliases */
	void Context::initAliases() {

		if (!m_sourcePath.ends_with("stdlib/src/coerce.bea"))
			m_aliases["Coerce"] = {"std", "coerce", "Coerce"};

		if (!m_sourcePath.ends_with("stdlib/src/iterator.bea")) {

			m_aliases["Iterable"] = {"std", "iterator", "Iterable"};
			m_aliases["Iterator"] = {"std", "iterator", "Iterator"};
		}
		if (!m_sourcePath.ends_with("stdlib/src/list.bea"))
			m_aliases["List"] = {"std", "list", "List"};

		if (!m_sourcePath.ends_with("stdlib/src/map.bea"))
			m_aliases["Map"] = {"std", "map", "Map"};

		if (!m_sourcePath.ends_with("stdlib/src/none.bea"))
			m_aliases["None"] = {"std", "none", "None"};

		if (!m_sourcePath.ends_with("stdlib/src/operator.bea")) {

			m_aliases["Add"] = {"std", "operator", "Add"};
			m_aliases["Subtract"] = {"std", "operator", "Subtract"};
			m_aliases["Multiply"] = {"std", "operator", "Multiply"};
			m_aliases["Divide"] = {"std", "operator", "Divide"};
			m_aliases["Modulo"] = {"std", "operator", "Modulo"};
			m_aliases["BitwiseOr"] = {"std", "operator", "BitwiseOr"};
			m_aliases["BitwiseAnd"] = {"std", "operator", "BitwiseAnd"};
			m_aliases["BitwiseXor"] = {"std", "operator", "BitwiseXor"};
			m_aliases["BitwiseLeftShift"] = {"std", "operator", "BitwiseLeftShift"};
			m_aliases["BitwiseRightShift"] = {"std", "operator", "BitwiseRightShift"};
			m_aliases["Equals"] = {"std", "operator", "Equals"};
			m_aliases["LessThan"] = {"std", "operator", "LessThan"};
			m_aliases["GreaterThan"] = {"std", "operator", "GreaterThan"};
			m_aliases["True"] = {"std", "operator", "True"};
			m_aliases["Index"] = {"std", "operator", "Index"};
		}
		if (!m_sourcePath.ends_with("stdlib/src/range.bea"))
			m_aliases["Range"] = {"std", "range", "Range"};

		if (!m_sourcePath.ends_with("stdlib/src/result.bea")) {

			m_aliases["Error"] = {"std", "result", "Error"};
			m_aliases["Result"] = {"std", "result", "Result"};
		}

		if (!m_sourcePath.ends_with("stdlib/src/set.bea"))
			m_aliases["Set"] = {"std", "set", "Set"};
	}

	/* Reference printer */
	std::ostream& operator<<(
		std::ostream& stream,
		const Context::Reference& reference
	) {
		stream << reference.position << ":" << reference.name << "(";
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
