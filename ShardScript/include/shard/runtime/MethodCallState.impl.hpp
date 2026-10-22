#pragma once
#include <shard/runtime/MethodCallState.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Strings.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace shard::detail
{
	inline bool unwrap_arg(ObjectInstance value, bool*)
	{
		return value.as_boolean();
	}

	inline std::int64_t unwrap_arg(ObjectInstance value, std::int64_t*)
	{
		return value.as_integer();
	}

	inline double unwrap_arg(ObjectInstance value, double*)
	{
		return value.as_double();
	}

	inline wchar_t unwrap_arg(ObjectInstance value, wchar_t*)
	{
		return value.as_char();
	}

	inline std::uint8_t unwrap_arg(ObjectInstance value, std::uint8_t*)
	{
		return static_cast<std::uint8_t>(value.as_integer());
	}

	inline ObjectInstance unwrap_arg(ObjectInstance value, ObjectInstance*)
	{
		return value;
	}

	inline std::wstring_view unwrap_string(ObjectInstance instance)
	{
		if (instance.is_null())
			return std::wstring_view();

		std::int64_t length = instance.as_integer();
		const wchar_t* data = reinterpret_cast<const wchar_t*>(instance.get_memory() + sizeof(std::int64_t));
		return std::wstring_view(data, static_cast<std::size_t>(length));
	}

	inline std::wstring_view unwrap_arg(ObjectInstance value, std::wstring_view*)
	{
		return unwrap_string(value);
	}

	inline gmt::wstring unwrap_arg(ObjectInstance value, gmt::wstring*)
	{
		return gmt::wstring(unwrap_string(value));
	}

	inline gmt::string unwrap_arg(ObjectInstance value, gmt::string*)
	{
		return gmt::wide_to_utf8(unwrap_string(value));
	}

	template<typename T>
	inline T* unwrap_arg(ObjectInstance value, T**)
	{
		if (value.is_null())
			return nullptr;

		return static_cast<T*>(value.as_pointer());
	}

	template<typename T>
	inline T unwrap_argument_at(const MethodCallState& state, std::size_t index)
	{
		ObjectInstance value = state.get_argument(index);
		T* tag = nullptr;
		return unwrap_arg(value, tag);
	}

	template<typename... TArgs, std::size_t... Indices>
	inline std::tuple<TArgs...> get_arguments_impl(const MethodCallState& state, std::index_sequence<Indices...>)
	{
		return std::make_tuple(unwrap_argument_at<TArgs>(state, Indices)...);
	}
}

namespace shard
{
	// Extracts the method arguments into a typed tuple. Supported types: bool,
	// std::int64_t, double, wchar_t, std::uint8_t, std::wstring_view,
	// gmt::wstring, ObjectInstance, and any native pointer T*.
	template<typename... TArgs>
	inline std::tuple<TArgs...> MethodCallState::get_arguments() const
	{
		constexpr std::size_t expectedCount = sizeof...(TArgs);
		if (get_argument_count() != expectedCount)
		{
			throw std::invalid_argument(
				std::string("expected ") + std::to_string(expectedCount) +
				" arguments, got " + std::to_string(get_argument_count()));
		}

		return detail::get_arguments_impl<TArgs...>(*this, std::index_sequence_for<TArgs...>{});
	}
}
