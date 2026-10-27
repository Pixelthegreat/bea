#pragma once

#include <iostream>
#include <initializer_list>
#include <unordered_map>
#include <bea/core.hpp>

namespace bea {

	/*
	 * Fixed size dynamically allocated array
	 */
	template <typename T>
	class DynFixed {
	public:
		DynFixed(std::size_t size)
			: m_size(size) {

			if (m_size) m_items = new T[m_size];
		}

		DynFixed(std::initializer_list<T> list)
			: m_size(list.size()) {

			if (m_size) m_items = new T[m_size];

			std::size_t index = 0;
			for (const T& item: list)
				m_items[index++] = item;
		}

		DynFixed(const std::vector<T>& vector)
			: m_size(vector.size()) {

			if (m_size) m_items = new T[m_size];

			std::size_t index = 0;
			for (const T& item: vector)
				m_items[index++] = item;
		}

		DynFixed(const DynFixed<T>& other) {

			if (m_items) delete[] m_items;

			m_size = other.m_size;
			m_items = new T[m_size];

			std::size_t index = 0;
			for (const T& item: other)
				m_items[index++] = item;
		}

		DynFixed(DynFixed<T>&& other) {

			if (m_items) delete[] m_items;

			m_items = other.m_items;
			m_size = other.m_size;

			other.m_items = nullptr;
			other.m_size = 0;
		}

		DynFixed<T>& operator=(const DynFixed<T>& other) {

			if (m_items) delete[] m_items;

			m_size = other.m_size;
			m_items = new T[m_size];

			std::size_t index = 0;
			for (const T& item: other)
				m_items[index++] = item;

			return *this;
		}

		DynFixed<T>& operator=(DynFixed<T>&& other) {

			if (m_items) delete[] m_items;

			m_items = other.m_items;
			m_size = other.m_size;

			other.m_items = nullptr;
			other.m_size = 0;

			return *this;
		}

		~DynFixed() {

			if (m_items) delete[] m_items;
			m_items = nullptr;
		}

		T operator[](std::size_t index) const { return m_items[index]; }
		T& operator[](std::size_t index) { return m_items[index]; }

		const T *data() const { return m_items; }
		const std::size_t size() const { return m_size; }
		const bool empty() const { return !m_size; }
		const T *begin() const { return m_items; }
		const T *end() const { return m_items+m_size; }
	private:
		T *m_items = nullptr;
		std::size_t m_size = 0;
	};

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
		const std::pair<K, V> *data() const { return m_items.data(); }
		const std::size_t size() const { return m_items.size(); }
		const bool empty() const { return m_items.empty(); }
		const std::pair<K, V> *begin() const { return m_items.data(); }
		const std::pair<K, V> *end() const { return m_items.data() + m_items.size(); }

		/*
		 * Get entry in map
		 */
		V& operator[](const K& key) {

			for (std::pair<K, V>& pair: m_items) {
				if (pair.first == key)
					return pair.second;
			}
			std::pair<K, V> pair{};
			pair.first = key;

			m_items.push_back(pair);

			return m_items[m_items.size()-1].second;
		}
		V& operator[](K&& key) {

			for (std::pair<K, V>& pair: m_items) {
				if (pair.first == key)
					return pair.second;
			}
			std::pair<K, V> pair{};
			pair.first = std::move(key);

			m_items.push_back(std::move(pair));

			return m_items[m_items.size()-1].second;
		}
	private:
		std::vector<std::pair<K, V>> m_items;
	};
}
