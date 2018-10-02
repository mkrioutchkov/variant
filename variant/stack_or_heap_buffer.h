#pragma once
#include <algorithm>
#include <memory>

template<size_t N = 2048, typename T = char>
struct stack_or_heap_buffer
{
    stack_or_heap_buffer(const size_t sizeNeeded = 0)
    {
        static_assert(std::is_pod<T>::value, "Use primive or POD type");
        reset();
        resize(sizeNeeded);
    }
    
    template<size_t ANOTHER_N>
    stack_or_heap_buffer(stack_or_heap_buffer<ANOTHER_N, T>&& other, int = {})
    {
        other.move_to(*this);
    }
    
    template<size_t ANOTHER_N>
    stack_or_heap_buffer(const stack_or_heap_buffer<ANOTHER_N, T>& other, int = {}) : stack_or_heap_buffer(other.size())
    {
        other.copy_to(*this);
    }
    
    stack_or_heap_buffer(const stack_or_heap_buffer& other) : stack_or_heap_buffer<N>(other, int{}) {}
    
    stack_or_heap_buffer(stack_or_heap_buffer&& other) : stack_or_heap_buffer<N>(std::move(other), int{}) {}
    
    template<size_t ANOTHER_N>
    stack_or_heap_buffer& operator=(stack_or_heap_buffer<ANOTHER_N, T>&& other)
    {
        other.move_to(*this);
        return *this;
    }
       
    void resize(const size_t sizeNeeded)
    {
        if(sizeNeeded > m_capacity)
        {
            m_heapBuffer = std::make_unique<T[]>(sizeNeeded);
        }

        m_size = sizeNeeded;
        m_capacity = std::max(m_capacity, m_size);
    }
    
    bool acquire(std::unique_ptr<T[]> buffer, const size_t size)
    {
        if(m_heapBuffer = std::move(buffer); m_heapBuffer)
            m_capacity = m_size = size;
        return m_heapBuffer != nullptr;
    }

    operator T*()
    {
        return m_heapBuffer ? m_heapBuffer.get() : m_stackBuffer;
    }

    operator const T*() const
    {
        return m_heapBuffer ? m_heapBuffer.get() : m_stackBuffer;
    }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    
    template<size_t ANOTHER_N>
    void move_to(stack_or_heap_buffer<ANOTHER_N, T>& other)
    {
        if(!other.acquire(std::move(m_heapBuffer), m_size))
        {
            other.resize(size());
            copy_to(other);
            m_size = 0;
            return;
        }  
       
        reset();
    }
    
    template<size_t ANOTHER_N>
    void copy_to(stack_or_heap_buffer<ANOTHER_N, T>& other) const
    {
        std::copy(ptr(), ptr() + other.size(), static_cast<T*>(other) );
    }
    
    auto ptr() { return static_cast<T*>(*this); }
    auto ptr() const { return static_cast<const T*>(*this); }
    
private:
    void reset()
    {
        m_size = 0;
        m_capacity = N;
    }
        
    T m_stackBuffer[N];
    size_t m_size = 0;
    size_t m_capacity = N;
    std::unique_ptr<T[]> m_heapBuffer;
};
