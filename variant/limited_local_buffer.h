#pragma once
#include "type_trait_extensions.h"

#include <algorithm>
#include <memory>

namespace mdk
{
	// A buffer for allocating memory on the stack; until there is no longer enough space.
	// See variant for an example of usefulness. 
	template<size_t N = 2048, typename T = char>
	struct limited_local_buffer
	{
		limited_local_buffer(const size_t sizeNeeded = 0)
		{
			static_assert(std::is_pod<T>::value, "Use primive or POD type");
			reset();
			resize(sizeNeeded);
		}

		template<size_t ANOTHER_N>
		limited_local_buffer(limited_local_buffer<ANOTHER_N, T>&& other, int = {})
		{
			other.move_to(*this);
		}

		template<size_t ANOTHER_N>
		limited_local_buffer(const limited_local_buffer<ANOTHER_N, T>& other, int = {}) : limited_local_buffer(other.size())
		{
			other.copy_to(*this);
		}

		limited_local_buffer(const limited_local_buffer& other) : limited_local_buffer<N>(other, int{}) {}

		limited_local_buffer(limited_local_buffer&& other) : limited_local_buffer<N>(std::move(other), int{}) {}

		template<size_t ANOTHER_N>
		limited_local_buffer& operator=(limited_local_buffer<ANOTHER_N, T>&& other)
		{
			other.move_to(*this);
			return *this;
		}

		void resize(const size_t sizeNeeded)
		{
			if (sizeNeeded > m_capacity)
			{
				m_heapBuffer = std::make_unique<T[]>(sizeNeeded);
			}

			set_size(sizeNeeded);
		}

		bool acquire(std::unique_ptr<T[]> buffer, const size_t size) noexcept
		{
			if (m_heapBuffer = std::move(buffer); m_heapBuffer)
				set_size(size); //m_capacity = m_size = size;
			return m_heapBuffer != nullptr;
		}

		operator T*() noexcept
		{
			return m_heapBuffer ? m_heapBuffer.get() : m_stackBuffer;
		}

		operator const T*() const noexcept
		{
			return m_heapBuffer ? m_heapBuffer.get() : m_stackBuffer;
		}

		size_t size() const noexcept { return m_size; }
		size_t capacity() const noexcept { return m_capacity; }

		template<size_t ANOTHER_N>
		void move_to(limited_local_buffer<ANOTHER_N, T>& other)
		{
			if (!other.acquire(std::move(m_heapBuffer), m_size))
			{
				other.resize(size());
				copy_to(other);
				m_size = 0;
				return;
			}

			reset();
		}

		template<size_t ANOTHER_N>
		void copy_to(limited_local_buffer<ANOTHER_N, T>& other) const
		{
			std::copy(ptr(), ptr() + other.size(), static_cast<T*>(other));
		}

		auto ptr() noexcept { return static_cast<T*>(*this); }
		auto ptr() const noexcept { return static_cast<const T*>(*this); }

	private:
		void reset() noexcept
		{
			m_size = 0;
			m_capacity = N;
		}

		void set_size(const size_t size) noexcept
		{
			m_size = size;
			m_capacity = std::max(m_capacity, m_size);
		}

		T m_stackBuffer[N];
		size_t m_size = 0;
		size_t m_capacity = N;
		std::unique_ptr<T[]> m_heapBuffer;
	};
}
