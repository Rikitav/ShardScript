#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

namespace shard
{
    namespace detail
    {
        // -------------------------------------------------------------------------
        // Per-type argument extraction.
        // Overload resolution keeps this free of heavy metaprogramming.
        // -------------------------------------------------------------------------
        inline bool               UnwrapArg(ObjectInstance* value, bool*)               { return value->AsBoolean(); }
        inline std::int64_t       UnwrapArg(ObjectInstance* value, std::int64_t*)       { return value->AsInteger(); }
        inline double             UnwrapArg(ObjectInstance* value, double*)             { return value->AsDouble(); }
        inline wchar_t            UnwrapArg(ObjectInstance* value, wchar_t*)            { return value->AsCharacter(); }
        inline std::uint8_t       UnwrapArg(ObjectInstance* value, std::uint8_t*)       { return value->AsByte(); }
        inline const wchar_t*     UnwrapArg(ObjectInstance* value, const wchar_t**)     { return value->AsString(); }
        inline std::wstring       UnwrapArg(ObjectInstance* value, std::wstring*)       { return std::wstring(value->AsString()); }
        inline ObjectInstance*    UnwrapArg(ObjectInstance* value, ObjectInstance**)    { return value; }

        template<typename T>
        inline T* UnwrapArg(ObjectInstance* value, T**)
        {
            return static_cast<T*>(value->AsNint());
        }

        template<typename T>
        inline T UnwrapArgAtIndex(const CallState& context, std::size_t index)
        {
            if (index >= context.Args.size())
            {
                throw std::runtime_error("Argument index out of range");
            }

            ObjectInstance* value = context.Args[index];
            if (value == nullptr)
            {
                throw std::runtime_error("Argument is null");
            }

            // ObjectInstance* is allowed to be NullInstance (represents script 'null'),
            // but value types and native pointers cannot be unwrapped from null.
            if constexpr (!std::is_same_v<T, ObjectInstance*> && !std::is_pointer_v<T>)
            {
                if (value == GarbageCollector::NullInstance)
                {
                    throw std::runtime_error("Argument is NullInstance");
                }
            }

            T* tag = nullptr;
            return UnwrapArg(value, tag);
        }

        template<typename... Args, std::size_t... Indices>
        inline std::tuple<Args...> GetArgsImpl(const CallState& context, std::index_sequence<Indices...>)
        {
            return std::make_tuple(UnwrapArgAtIndex<Args>(context, Indices)...);
        }
    }

    /// <summary>
    /// Extracts the method arguments from a CallState into a typed tuple.
    /// Supported types: bool, std::int64_t, double, wchar_t, std::uint8_t,
    /// const wchar_t*, std::wstring, ObjectInstance*, and any native pointer T*.
    /// </summary>
    template<typename... Args>
    inline std::tuple<Args...> GetArgs(const CallState& context)
    {
        if (context.Args.size() != sizeof...(Args))
        {
            throw std::runtime_error(
                "Expected " + std::to_string(sizeof...(Args)) +
                " arguments, got " + std::to_string(context.Args.size()));
        }

        return detail::GetArgsImpl<Args...>(context, std::index_sequence_for<Args...>{});
    }
}
