#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <type_traits>

template <typename T>
struct is_unique_ptr_struct : std::false_type {};

template <typename T, typename D>
struct is_unique_ptr_struct<std::unique_ptr<T, D>> : std::true_type {};

template <typename T>
inline constexpr bool is_unique_ptr_v = is_unique_ptr_struct<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsUniquePtr = is_unique_ptr_v<T>;

template <typename T, size_t SparseMax = 2048, size_t Capacity = 1024>
class SparseSet
{
private:
	std::vector<size_t> m_denseToSparse;
	std::vector<size_t> m_sparse;
	std::vector<T> m_dense;
	size_t m_size{ 0 };

public:
	SparseSet()
	{
		m_sparse.resize(SparseMax, Capacity);
		m_dense.reserve(Capacity);
		m_denseToSparse.reserve(Capacity);
	}

	bool full()
	{
		return m_denseToSparse.size() == Capacity;
	}

	bool contains(size_t index)
	{
		return m_sparse[index] != Capacity;
	}

	void insert(size_t index, const T& value)
	{
		size_t c = Capacity;

		if (index >= SparseMax || m_size >= c)
		{
			throw std::out_of_range("Index out of bounds or capacity exceeded");
		}

		if (m_sparse[index] != Capacity)
		{
			throw std::runtime_error("Element already exists at this index");
		}

		m_dense.push_back(value);
		m_denseToSparse.push_back(index);
		m_sparse[index] = m_size;
		m_size++;
	}

	void insert(size_t index, T&& value)
	{
		if (index >= SparseMax || m_size >= Capacity)
		{
			throw std::out_of_range("Index out of bounds or capacity exceeded");
		}

		if (m_sparse[index] != Capacity)
		{
			throw std::runtime_error("Element already exists at this index");
		}

		m_dense.push_back(std::move(value));
		m_denseToSparse.push_back(index);
		m_sparse[index] = m_size;
		m_size++;
	}

	void remove(size_t index)
	{
		if (index >= SparseMax || m_sparse[index] == Capacity)
		{
			throw std::out_of_range("Index out of bounds or element does not exist");
		}
		size_t denseIndex = m_sparse[index];
		size_t lastSparseIndex = m_denseToSparse[m_size - 1];

		if constexpr (IsUniquePtr<T>)
		{
			m_dense[denseIndex] = std::move(m_dense[size - 1]);
		}

		else
		{
			m_dense[denseIndex] = m_dense[size - 1];
		}

		m_sparse[lastSparseIndex] = denseIndex;
		m_denseToSparse[denseIndex] = lastSparseIndex;

		m_sparse[index] = Capacity;
		m_dense.pop_back();
		m_denseToSparse.pop_back();
		m_size--;
	}

	T& get(size_t index)
	{
		if (index >= SparseMax || m_sparse[index] == Capacity)
		{
			throw std::out_of_range("Index out of bounds or element does not exist");
		}
		return m_dense[m_sparse[index]];
	}

	const T& get(size_t index) const
	{
		if (index >= SparseMax || m_sparse[index] == Capacity)
		{
			throw std::out_of_range("Index out of bounds or element does not exist");
		}
		return m_dense[m_sparse[index]];
	}

	auto begin()
	{
		return m_dense.begin();
	}

	auto end()
	{
		return m_dense.end();
	}

	auto begin() const
	{
		return m_dense.begin();
	}

	auto end() const
	{
		return m_dense.end();
	}

	auto cbegin() const
	{
		return m_dense.cbegin();
	}

	auto cend() const
	{
		return m_dense.cend();
	}

	size_t size()
	{
		return m_size;
	}

};