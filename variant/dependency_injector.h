#pragma once
#include "variant.h"
#include "tuple_extensions.h"

#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <utility>
#include <exception>

namespace mdk
{
	class dependency_injector
	{
		struct lazy_construct : std::function<void(variant&)>
		{
		};
	public:
		struct exception : std::exception
		{
			exception(const char* text) noexcept : m_text(text) {}
			const char* const m_text = nullptr;
		};

		template<typename T, typename... Args>
		T& try_emplace(Args&&... args) noexcept(false)
		{
			const auto result = m_variants.emplace(&typeid(T), T(std::forward<Args>(args)...));
			if (!result.second)
				throw exception("emplace failed");
			return result.first->second.template force_cast<T>();
		}

		template<typename T>
		bool lazy_emplace(lazy_construct creator) noexcept
		{
			const auto result = m_variants.emplace(&typeid(T), std::move(creator));
			return result.second;
		}

		template<typename T, typename TUPLE>
		bool lazy_emplace(TUPLE tuple) noexcept
		{
			lazy_construct creator{ { [tuple = std::move(tuple)](variant& v) {
				v = construct_from_tuple<T>(std::move(tuple));
			}} };
			return lazy_emplace<T>(std::move(creator));
		}

		template<typename T>
		const T* force_get() const noexcept
		{
			if (auto it = m_variants.find(&typeid(T)); it != std::end(m_variants))
				return &it->second.force_cast<T>();
			return nullptr;
		}

		template<typename T>
		const T* get() noexcept
		{
			if (auto it = m_variants.find(&typeid(T)); it != std::end(m_variants))
			{
				variant& v = it->second;
				if (auto ptr = v.cast<T>())
					return ptr;
				// if the variant doesn't store a T, then it must be storing a function that can create a T on demand
				auto& constructor = v.force_cast<lazy_construct>();
				constructor(v);
				return &v.force_cast<T>();
			}
			return nullptr;
		}
	private:
		std::unordered_map<const std::type_info*, variant> m_variants;
	};
}