#include <gmt/Interner.hpp>

using namespace gmt;

rs::stringintern::StringIntern& gmt::global_interner()
{
	static rs::stringintern::StringIntern instance;
	return instance;
}

rs::stringintern::StringReference gmt::intern(std::wstring_view text)
{
	return global_interner().Add(text.data(), text.length());
}

std::wstring_view gmt::resolve(rs::stringintern::StringReference ref)
{
	std::size_t length = 0;
	const wchar_t* str = global_interner().ToWString(ref, length);
	if (str == nullptr)
		return std::wstring_view();

	return std::wstring_view(str, length);
}
