#pragma once

#include <gmt/Containers.hpp>

#include <string>
#include <string_view>

namespace gmt
{
	template<typename... Args>
	[[nodiscard]] gmt::wstring wconcat(const Args&... args)
	{
		const std::size_t total_size = (std::size_t() + ... + std::wstring_view(args).size());
		gmt::wstring result;
		result.reserve(total_size);
		(result.append(args), ...);
		return result;
	}

	template<typename... Args>
	[[nodiscard]] gmt::string concat(const Args&... args)
	{
		const std::size_t total_size = (std::size_t() + ... + std::string_view(args).size());
		gmt::string result;
		result.reserve(total_size);
		(result.append(args), ...);
		return result;
	}
}
