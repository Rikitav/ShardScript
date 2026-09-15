#pragma once

namespace gmt
{
	struct nullref_t
	{
		explicit constexpr nullref_t(int) { }
	};

	inline constexpr nullref_t nullref{ nullref_t{ 0 } };
}
