#pragma once
#include <shard/Definitions.hpp>
#include <shard/LibraryMetadata.hpp>

#include <shard/runtime/MethodCallState.hpp>

#include <gmt/Strings.hpp>

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace shard::detail
{
	template<typename T, typename... Set>
	inline constexpr bool is_one_of_v = (std::is_same_v<T, Set> || ...);

	template<typename T>
	inline constexpr bool is_string_like_v = is_one_of_v<T,
		std::wstring, std::wstring_view, const std::wstring&,
		std::string, std::string_view, const std::string&>;

	template<typename T>
	inline constexpr bool is_allowed_arg_v =
		is_one_of_v<T, bool, std::int64_t, double, wchar_t, std::uint8_t, ObjectInstance>
		|| is_string_like_v<T>
		|| std::is_pointer_v<T>;

	template<typename T>
	inline constexpr bool is_allowed_return_v =
		std::is_void_v<T>
		|| is_one_of_v<T, bool, std::int64_t, double, wchar_t, std::uint8_t, ObjectInstance, const wchar_t*>
		|| is_string_like_v<T>
		|| std::is_pointer_v<T>;

	// Storage type used to carry an argument from the tuple into the wrapped
	// function: by-value forms materialize converted strings, references and
	// views bind to the stored element for the duration of the call.
	template<typename T>
	struct arg_storage
	{
		using type = T;
	};

	template<>
	struct arg_storage<std::wstring>
	{
		using type = std::wstring;
	};

	template<>
	struct arg_storage<const std::wstring&>
	{
		using type = std::wstring;
	};

	template<>
	struct arg_storage<std::string>
	{
		using type = std::string;
	};

	template<>
	struct arg_storage<const std::string&>
	{
		using type = std::string;
	};

	template<>
	struct arg_storage<std::string_view>
	{
		using type = std::string;
	};

	template<typename T>
	using arg_storage_t = typename arg_storage<T>::type;

	template<typename R>
	void set_native_return(MethodCallState& state, R&& value)
	{
		using D = std::decay_t<R>;

		if constexpr (std::is_same_v<D, bool>)
			state.set_return_boolean(value);
		else if constexpr (std::is_same_v<D, std::int64_t>)
			state.set_return_integer(value);
		else if constexpr (std::is_same_v<D, double>)
			state.set_return_double(value);
		else if constexpr (std::is_same_v<D, wchar_t>)
			state.set_return_char(value);
		else if constexpr (std::is_same_v<D, std::uint8_t>)
			state.set_return_byte(value);
		else if constexpr (std::is_same_v<D, ObjectInstance>)
			state.set_return(value);
		else if constexpr (std::is_same_v<D, std::wstring> || std::is_same_v<D, std::wstring_view>)
			state.set_return_string(value);
		else if constexpr (std::is_same_v<D, const wchar_t*>)
			state.set_return_string(value);
		else if constexpr (std::is_same_v<D, std::string> || std::is_same_v<D, std::string_view>)
			state.set_return_string(gmt::utf8_to_wide(value));
		else
			static_assert(!sizeof(D*), "unsupported native return type");
	}
}

namespace shard
{
	// Maps a cdecl function onto a MethodSymbolDelegate at compile time:
	// arguments are unpacked through MethodCallState::get_arguments and the
	// result is placed with the matching set_return_* overload.
	// Allowed argument/return types: bool, std::int64_t, double, wchar_t,
	// std::uint8_t, ObjectInstance, string types (wide zero-copy, narrow via
	// utf-8 conversion), native pointers, and void returns.
	template<auto Func>
	struct native_wrapper;

	template<typename R, typename... Args, R(*Func)(Args...)>
	struct native_wrapper<Func>
	{
		static_assert((detail::is_allowed_arg_v<Args> && ...), "unsupported native argument type");
		static_assert(detail::is_allowed_return_v<R>, "unsupported native return type");

		static void invoke(MethodCallState& state)
		{
			std::tuple<detail::arg_storage_t<Args>...> args = state.get_arguments<detail::arg_storage_t<Args>...>();

			if constexpr (std::is_void_v<R>)
			{
				std::apply(Func, args);
				return;
			}
			else
			{
				detail::set_native_return(state, std::apply(Func, args));
			}
		}
	};

	template<auto Func>
	inline constexpr MethodSymbolDelegate wrap_native = &native_wrapper<Func>::invoke;
}
