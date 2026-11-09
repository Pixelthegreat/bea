#pragma once

#include <iostream>
#include <initializer_list>
#include <unordered_map>
#include <bea/core.hpp>

namespace bea {

	/*
	 * Two items of the same type
	 */
	template <typename T>
	using Pair = std::array<T, 2>;

	/*
	 * Map that maintains insertion order
	 */
	template <typename K, typename V>
	class OrderedMap {
	public:
		/*
		 * Iterator
		 */
		std::pair<K, V> *data() { return m_items.data(); }
		std::size_t size() const { return m_items.size(); }
		bool empty() const { return m_items.empty(); }
		std::pair<K, V> *begin() { return m_items.data(); }
		std::pair<K, V> *end() { return m_items.data() + m_items.size(); }

		const std::pair<K, V> *data() const { return m_items.data(); }
		const std::pair<K, V> *begin() const { return m_items.data(); }
		const std::pair<K, V> *end() const { return m_items.data() + m_items.size(); }

		/*
		 * Get entry in map
		 */
		Result<V *> get(const K& key) {

			std::size_t hash = std::hash<K>{}(key);

			for (auto it = m_items.rbegin(); it != m_items.rend(); it++) {

				std::pair<K, V>& pair = *it;
				if (std::hash<K>{}(pair.first) == hash)
					return Result<V *>(&pair.second);
			}
			return Result<V *>::fail();
		}
		Result<V *> get(K&& key) {

			std::size_t hash = std::hash<K>{}(key);

			for (auto it = m_items.rbegin(); it != m_items.rend(); it++) {

				std::pair<K, V>& pair = *it;
				if (std::hash<K>{}(pair.first) == hash)
					return Result<V *>(&pair.second);
			}
			return Result<V *>::fail();
		}

		V& operator[](const K& key) {

			Result<V *> result = get(key);
			if (result.okay) return *result.value;

			return newEntry(key);
		}
		V& operator[](K&& key) {

			Result<V *> result = get(key);
			if (result.okay) return *result.value;

			return newEntry(std::move(key));
		}

		/*
		 * Create new entry at end
		 */
		V& newEntry(const K& key) {

			std::pair<K, V> pair{};
			pair.first = key;

			m_items.push_back(std::move(pair));

			return m_items[m_items.size()-1].second;
		}
		V& newEntry(K&& key) {

			std::pair<K, V> pair{};
			pair.first = std::move(key);

			m_items.push_back(std::move(pair));

			return m_items[m_items.size()-1].second;
		}
	private:
		std::vector<std::pair<K, V>> m_items;
	};
}
