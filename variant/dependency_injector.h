#pragma once
#include "variant.h"

#include <unordered_map>
#include <typeinfo>
#include <functional>

namespace mdk
{
	class dependency_injector
	{
		struct lazy_construct : std::function<void(void)>
		{
		};
	public:
		template<typename T, typename... Args>
		T& emplace(Args&&... args)
		{
			const auto result = m_variants.emplace(std::piecewise_construct, std::forward_as_tuple(&typeid(T)), std::forward_as_tuple(std::forward<Args>(args)...));
			return result.first->second.template force_cast<T>();
		}

		template<typename T>
		bool lazy_emplace(lazy_construct creator)
		{
			const auto result = m_variants.emplace(&typeid(T), creator);
			return result.second;
		}

		template<typename T>
		const T* force_get() const
		{
			if (auto it = m_variants.find(&typeid(T)); it != std::end(m_variants))
				return &it->second.force_cast<T>();
			return nullptr;
		}
	private:
		std::unordered_map<const std::type_info*, variant> m_variants;
	};
}