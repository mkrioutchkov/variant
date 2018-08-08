#pragma once
#include "stack_or_heap_buffer.h"

#include <iostream>
#include <type_traits>
#include <utility>

using buffer_t = stack_or_heap_buffer<>;

struct variant_operation_holder_i
{
    virtual const std::type_info& type_info() const = 0;
    
    virtual void destroy(buffer_t& dst) const = 0;
    
    virtual void copy_construct(buffer_t&, const buffer_t&) const = 0;
    virtual void copy_assign(buffer_t&, const buffer_t&) const = 0;
    
    virtual void move_construct(buffer_t&, buffer_t&) const = 0;
    virtual void move_assign(buffer_t&, buffer_t&) const = 0;
    
    virtual std::ostream& out_stream(std::ostream& stream, const buffer_t& src) const = 0;

    // 1. Checks if dst and src are the same type
    // 2. If yes, then stops here
    // 3. Calls destructor at dst
    // 4. If dst needs more space, then reallocates
    void repurpose(buffer_t& dst, const variant_operation_holder_i& src_var) const
    {
        if(type_info() == src_var.type_info())
            return; // nothing to do
        
        destroy(dst);
        src_var.build(dst);
    }
    
    void build(buffer_t& dst) const
    {
        dst.resize(size());
        construct(dst);
    } 
    
protected:
    virtual size_t size() const = 0;
    virtual void construct(buffer_t& dst) const = 0;
};

template<typename T>
struct variant_operation_holder : variant_operation_holder_i
{
    static auto& instance()
    {
        const static variant_operation_holder operations;
        return operations;
    }
    
    const std::type_info& type_info() const override final
    {
        return typeid(T);
    }
    
    void destroy(buffer_t& dst) const override final
    {
        lvalue(dst).~T();
    }
    
    void copy_construct(buffer_t& dst, const buffer_t& src) const override final
    {
        new (dst) T(lvalue(src));
    }
    
    void copy_assign(buffer_t& dst, const buffer_t& src) const override final
    {
        if constexpr(std::is_copy_assignable_v<T>)
            lvalue(dst) = lvalue(src);
    }
    
    void move_construct(buffer_t& dst, buffer_t& src) const override final
    {
        new (dst) T(rvalue(src));
    }
    
    void move_assign(buffer_t& dst, buffer_t& src) const override final
    {
        if constexpr(std::is_move_assignable_v<T>)
            lvalue(dst) = rvalue(src);
    }
       
    std::ostream& out_stream(std::ostream& stream, const buffer_t& src) const override final
    {
        if constexpr(test_streamable<decltype(stream), decltype(src)>(bool{}))
            stream << "streaming a " << type_info().name() << ' ' << lvalue(src);
        else
            stream << "variant of type " << type_info().name();
            
        return stream;
    }
protected:
    size_t size() const override final { return sizeof(T); }
    
    void construct(buffer_t& dst) const override final
    {
        new (dst) T();
    }
private:
    variant_operation_holder() = default;    
    
    static T& lvalue(buffer_t& p)
    {
        return static_cast<T&>(*reinterpret_cast<T*>(static_cast<char*>(p) ));
    }
    
    static const T& lvalue(const buffer_t& p)
    {
        return static_cast<const T&>(*reinterpret_cast<const T*>( static_cast<const char*>(p) ));
    }
    
    static T&& rvalue(buffer_t& p)
    {
        return std::move(*reinterpret_cast<T*>( static_cast<char*>(p) ));
    }
    
    template<typename SS, typename TT>
    static constexpr auto test_streamable(bool) -> decltype(std::declval<SS>() << std::declval<TT>()) { return true; }
    
    template<typename... Args>
    static constexpr auto test_streamable(int) { return false; }
};

struct variant
{
    template<typename T, typename RAW_T = std::remove_cvref_t<T>, typename = std::enable_if_t<!std::is_same_v<RAW_T, variant> > >
    variant(T&& value) 
    :   m_type_info(&variant_operation_holder<RAW_T>::instance()), 
        m_buffer(sizeof(RAW_T))
    {
        static_assert(std::is_same_v<RAW_T, std::remove_cvref_t<T>>, "Don't explicitely pass RAW_T");
        new (m_buffer) RAW_T(std::forward<T>(value));
    }
    
    variant(variant&& other) 
    :   m_type_info(other.m_type_info)
    {
        m_type_info->move_construct(m_buffer, other.m_buffer);
    }
    
    variant(const variant& other) 
    :   m_type_info(other.m_type_info), 
        m_buffer(other.m_buffer.size())
    {
        m_type_info->copy_construct(m_buffer, other.m_buffer);    
    }

    ~variant()
    {
        m_type_info->destroy(m_buffer);
    }
    
    variant& operator=(variant&& other)
    {
        m_type_info->repurpose(m_buffer, *other.m_type_info);
        m_type_info = other.m_type_info;
        m_type_info->move_assign(m_buffer, other.m_buffer);
        return *this;
    }
    
    variant& operator=(const variant& other)
    {
        return operator=(variant(other));
    }

    std::ostream& out_stream(std::ostream& stream) const
    {
        return m_type_info->out_stream(stream, m_buffer);
    }
    
    template<typename T>
    operator T*()
    {
        return value<T>(*this);
    }
    
    template<typename T>
    operator const T*() const
    {
        return value<T>(*this);
    }
private:
    template<typename T, typename VARIANT>
    static auto value(VARIANT& this_variant)
    {
        using C_T = std::remove_reference_t<T>;
        using RAW_T = std::remove_cvref_t<T>;
        if(typeid(RAW_T) == this_variant.m_type_info->type_info())
            return reinterpret_cast<C_T*>(this_variant.m_buffer.ptr());
            
        return (C_T*){};
    }

    const variant_operation_holder_i* m_type_info;
    buffer_t m_buffer;
};

inline std::ostream& operator<<(std::ostream& stream, const variant& var)
{
    return var.out_stream(stream);
}