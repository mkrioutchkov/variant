#pragma once
#include <type_traits>

namespace mdk
{
	// update to std:: eventually
	template<typename T>
	using remove_cvref_t = std::remove_const_t<std::remove_reference_t<T>>;

	template<typename SS, typename TT>
	static constexpr auto test_streamable(bool) noexcept 
		-> decltype(((std::declval<SS>() << std::declval<TT>()), bool{})) { return true; }

	template<typename...>
	static constexpr auto test_streamable(int) noexcept { return false; }
}
