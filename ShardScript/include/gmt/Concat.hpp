#pragma once

#include <string>
#include <string_view>

namespace gmt
{
	template<typename... Args>
	[[nodiscard]] std::wstring concat(const Args&... args)
	{
		const std::size_t total_size = (std::size_t() + ... + std::wstring_view(args).size());
		std::wstring result;
		result.reserve(total_size);
		(result.append(args), ...);
		return result;
	}
}
