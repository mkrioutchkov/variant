#pragma once
#include "variant.h"
#include "tuple_extensions.h"

#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <utility>
#include <exception>
#include <typeindex>

namespace mdk
{
    class dependency_injector
    {
        struct lazy_construct : std::function<variant(void)>
        {
        };
    public:
        struct exception : std::exception
        {
            exception(const char* text) noexcept : m_text(text) {}
            const char* const m_text = nullptr;
        };

        template<typename T>
        std::pair<T&, bool> insert(T&& value)
        {
            return try_emplace<T>(std::forward<T>(value));
        }

        template<typename T>
        std::pair<T&, bool> insert(T& value)
        {
            return try_emplace<T>(std::forward<T>(value));
        }
        
        template<typename T, typename... Args>
        std::pair<T&, bool> try_emplace(Args&&... args)
        {
            const auto result = m_variants.emplace(typeid(T), T(std::forward<Args>(args)...));
            return { result.first->second.template force_cast<T>(), result.second };
        }

        template<typename T>
        bool lazy_emplace(lazy_construct creator) noexcept
        {
            const auto result = m_variants.emplace(typeid(T), std::move(creator));
            return result.second;
        }

        template<typename T, typename TUPLE>
        bool lazy_emplace(TUPLE tuple) noexcept
        {
            lazy_construct creator{ { [tuple = std::move(tuple)]() -> variant {
                return construct_from_tuple<T>(std::move(tuple));
            }} };
            return lazy_emplace<T>(std::move(creator));
        }

        template<typename T>
        const T* force_get() const noexcept
        {
            if (auto it = m_variants.find(typeid(T)); it != std::end(m_variants))
                return &it->second.force_cast<T>();
            return nullptr;
        }

        template<typename T>
        const T* get()
        {
            auto it = m_variants.find(typeid(T));
            if (it == std::end(m_variants))
                return nullptr;

            variant& v = it->second;
            if (auto ptr = v.cast<T>())
                return ptr;
            // if the variant doesn't store a T, then it must be storing a function that can create a T
            auto& constructor = v.force_cast<lazy_construct>();
            v = constructor();
            return &v.force_cast<T>();
        }
    private:
        std::unordered_map<std::type_index, variant> m_variants;
    };
}