#pragma once

#include <tuple>
#include <type_traits>

namespace mdk
{
    template <typename T, typename Tuple, size_t... Is>
    T construct_from_tuple(Tuple&& tuple, std::index_sequence<Is...>)
    {
        return T{ std::get<Is>(std::forward<Tuple>(tuple))... };
    }

    template <typename T, typename Tuple>
    T construct_from_tuple(Tuple&& tuple) 
    {
        return construct_from_tuple<T>(std::forward<Tuple>(tuple),
            std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{}
        );
    }
}

