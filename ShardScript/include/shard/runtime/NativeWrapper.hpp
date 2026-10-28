#pragma once
#include <shard/Definitions.hpp>
#include <shard/LibraryMetadata.hpp>

#include <gmt/Containers.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Strings.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace shard::detail
{
	template<typename T, typename... Set>
	inline constexpr bool is_one_of_v = (std::is_same_v<T, Set> || ...);

	template<typename T>
	using bare_t = std::remove_cvref_t<T>;

	template<typename T>
	struct is_basic_string : std::false_type { };

	template<typename CharT, typename Traits, typename Alloc>
	struct is_basic_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type { };

	template<typename T>
	struct is_basic_wstring : std::false_type { };

	template<typename Traits, typename Alloc>
	struct is_basic_wstring<std::basic_string<wchar_t, Traits, Alloc>> : std::true_type { };

	template<typename T>
	struct is_basic_narrow_string : std::false_type { };

	template<typename Traits, typename Alloc>
	struct is_basic_narrow_string<std::basic_string<char, Traits, Alloc>> : std::true_type { };

	template<typename T>
	inline constexpr bool is_wstring_v =
		is_one_of_v<bare_t<T>, std::wstring_view> || is_basic_wstring<bare_t<T>>::value;

	template<typename T>
	inline constexpr bool is_string_v =
		is_one_of_v<bare_t<T>, std::string_view> || is_basic_narrow_string<bare_t<T>>::value;

	template<typename T>
	inline constexpr bool is_stdlib_narrow_string_v =
		std::is_same_v<bare_t<T>, std::string>;

	template<typename T>
	inline constexpr bool is_stdlib_wide_string_v =
		std::is_same_v<bare_t<T>, std::wstring>;

	template<typename T>
	inline constexpr bool is_wchar_pointer_v =
		is_one_of_v<bare_t<T>, const wchar_t*, wchar_t*>;

	template<typename T>
	inline constexpr bool is_char_pointer_v =
		is_one_of_v<bare_t<T>, const char*, char*>;

	template<typename T>
	inline constexpr bool is_string_arg_v =
		is_wstring_v<T> || is_string_v<T> || is_wchar_pointer_v<T> || is_char_pointer_v<T>;

	template<typename T>
	inline constexpr bool is_object_pointer_v =
		std::is_pointer_v<bare_t<T>> && !std::is_function_v<std::remove_pointer_t<bare_t<T>>>;

	// Supported arguments: any arithmetic type (narrowed or widened by cast),
	// enums, native pointers, ObjectInstance and every string form above.
	template<typename T>
	inline constexpr bool is_allowed_arg_v =
		is_string_arg_v<T>
		|| std::is_same_v<bare_t<T>, ObjectInstance>
		|| is_object_pointer_v<T>
		|| std::is_arithmetic_v<bare_t<T>>
		|| std::is_enum_v<bare_t<T>>;

	// Supported returns: void, arithmetic, enums, strings, ObjectInstance and
	// native pointers (returned as an opaque integer handle).
	template<typename T>
	inline constexpr bool is_allowed_return_v =
		std::is_void_v<T>
		|| is_string_arg_v<T>
		|| std::is_same_v<bare_t<T>, ObjectInstance>
		|| is_object_pointer_v<T>
		|| std::is_arithmetic_v<bare_t<T>>
		|| std::is_enum_v<bare_t<T>>;

	// Storage type carried in the argument tuple. Strings are stored owning so
	// their buffers stay alive for the whole native call; everything else is
	// decayed, so const/volatile/reference qualifiers never reach the tuple.
	template<typename T>
	struct arg_storage
	{
		using bare = bare_t<T>;
		using type = std::conditional_t<
			is_wstring_v<bare> || is_wchar_pointer_v<bare>,
			gmt::wstring,
			std::conditional_t<
				is_string_v<bare> || is_char_pointer_v<bare>,
				gmt::string,
				bare>>;
	};

	template<typename T>
	using arg_storage_t = typename arg_storage<T>::type;

	// Reads one managed argument and casts it into the storage type
	template<typename Storage>
	Storage read_native_argument(const MethodCallState& state, std::size_t index)
	{
		if constexpr (is_wstring_v<Storage>)
		{
			const std::wstring_view view = state.get_string_argument(index);
			return Storage(view.data(), view.size());
		}
		else if constexpr (is_string_v<Storage>)
		{
			const gmt::string converted = gmt::wide_to_utf8(state.get_string_argument(index));
			return Storage(converted.data(), converted.size());
		}
		else if constexpr (std::is_same_v<Storage, ObjectInstance>)
		{
			return state.get_argument(index);
		}
		else if constexpr (std::is_same_v<Storage, bool>)
		{
			return state.get_boolean_argument(index);
		}
		else if constexpr (std::is_same_v<Storage, wchar_t> || std::is_same_v<Storage, char>)
		{
			return static_cast<Storage>(state.get_char_argument(index));
		}
		else if constexpr (std::is_floating_point_v<Storage>)
		{
			return static_cast<Storage>(state.get_double_argument(index));
		}
		else if constexpr (std::is_enum_v<Storage>)
		{
			return static_cast<Storage>(state.get_integer_argument(index));
		}
		else if constexpr (std::is_integral_v<Storage>)
		{
			return static_cast<Storage>(state.get_integer_argument(index));
		}
		else if constexpr (std::is_pointer_v<Storage>)
		{
			ObjectInstance value = state.get_argument(index);
			if ((value == nullptr))
				return nullptr;

			return reinterpret_cast<Storage>(value.as_pointer());
		}
		else
		{
			static_assert(!sizeof(Storage*), "unsupported native argument type");
		}
	}

	template<typename... Args, std::size_t... Indices>
	std::tuple<arg_storage_t<Args>...> read_native_arguments(
		const MethodCallState& state,
		std::index_sequence<Indices...>)
	{
		return std::make_tuple(read_native_argument<arg_storage_t<Args>>(state, Indices)...);
	}

	// Converts a stored value into the exact type the native signature expects
	template<typename Arg, typename Storage>
	decltype(auto) native_argument(Storage&& stored)
	{
		using BareArg = bare_t<Arg>;

		if constexpr (is_wchar_pointer_v<BareArg>)
		{
			if constexpr (std::is_const_v<std::remove_pointer_t<BareArg>>)
				return static_cast<const wchar_t*>(stored.c_str());
			else
				return const_cast<wchar_t*>(stored.c_str());
		}
		else if constexpr (is_char_pointer_v<BareArg>)
		{
			if constexpr (std::is_const_v<std::remove_pointer_t<BareArg>>)
				return static_cast<const char*>(stored.c_str());
			else
				return const_cast<char*>(stored.c_str());
		}
		else if constexpr (is_stdlib_wide_string_v<BareArg>)
		{
			// the storage uses gmt::allocator; a native function that asks for
			// a std::wstring gets a copy it can own
			return std::wstring(stored.data(), stored.size());
		}
		else if constexpr (is_stdlib_narrow_string_v<BareArg>)
		{
			return std::string(stored.data(), stored.size());
		}
		else
		{
			return static_cast<Arg>(std::forward<Storage>(stored));
		}
	}

	template<typename R>
	void set_native_return(MethodCallState& state, R&& value)
	{
		using D = bare_t<R>;

		if constexpr (std::is_same_v<D, bool>)
		{
			state.set_return_boolean(value);
		}
		else if constexpr (std::is_same_v<D, ObjectInstance>)
		{
			state.set_return(value);
		}
		else if constexpr (is_wstring_v<D>)
		{
			state.set_return_string(value);
		}
		else if constexpr (is_string_v<D>)
		{
			state.set_return_string(gmt::utf8_to_wide(value));
		}
		else if constexpr (is_wchar_pointer_v<D>)
		{
			state.set_return_string(value != nullptr ? std::wstring_view(value) : std::wstring_view());
		}
		else if constexpr (is_char_pointer_v<D>)
		{
			state.set_return_string(value != nullptr ? gmt::utf8_to_wide(value) : gmt::wstring());
		}
		else if constexpr (std::is_same_v<D, wchar_t> || std::is_same_v<D, char>)
		{
			state.set_return_char(static_cast<wchar_t>(value));
		}
		else if constexpr (std::is_enum_v<D>)
		{
			state.set_return_integer(static_cast<std::int64_t>(value));
		}
		else if constexpr (std::is_same_v<D, std::uint8_t>)
		{
			state.set_return_byte(value);
		}
		else if constexpr (std::is_integral_v<D>)
		{
			state.set_return_integer(static_cast<std::int64_t>(value));
		}
		else if constexpr (std::is_floating_point_v<D>)
		{
			state.set_return_double(static_cast<double>(value));
		}
		else if constexpr (std::is_pointer_v<D>)
		{
			// native pointers have no managed shape; expose them as an opaque
			// integer handle, mirroring how pointer arguments are passed in
			state.set_return_integer(reinterpret_cast<std::int64_t>(value));
		}
		else
		{
			static_assert(!sizeof(D*), "unsupported native return type");
		}
	}
}

namespace shard
{
	// Maps a cdecl function onto a MethodSymbolDelegate at compile time
	template<auto Func>
	struct native_wrapper;

	template<typename R, typename... Args, R(*Func)(Args...)>
	struct native_wrapper<Func>
	{
		static_assert((detail::is_allowed_arg_v<Args> && ...), "unsupported native argument type");
		static_assert(detail::is_allowed_return_v<R>, "unsupported native return type");

		static void invoke(MethodCallState& state)
		{
			constexpr std::size_t expectedCount = sizeof...(Args);
			if (state.get_argument_count() != expectedCount)
			{
				throw std::invalid_argument(
					std::string("expected ") +
					std::to_string(expectedCount) +
					" arguments, got " +
					std::to_string(state.get_argument_count())
				);
			}

			std::tuple<detail::arg_storage_t<Args>...> args =
				detail::read_native_arguments<Args...>(state, std::index_sequence_for<Args...>{});

			auto call = [&](auto&&... stored) -> decltype(auto)
			{
				return Func(detail::native_argument<Args>(std::forward<decltype(stored)>(stored))...);
			};

			if constexpr (std::is_void_v<R>)
			{
				std::apply(call, std::move(args));
			}
			else
			{
				detail::set_native_return(state, std::apply(call, std::move(args)));
			}
		}
	};

	template<auto Func>
	inline constexpr MethodSymbolDelegate wrap_native = &native_wrapper<Func>::invoke;
}
