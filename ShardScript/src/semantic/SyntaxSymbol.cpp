#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

#include <string>
#include <vector>

using namespace shard;

SyntaxSymbol::SyntaxSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: m_kind(kind), m_parent(parent), m_name(gmt::intern(name)) { }

SyntaxSymbol::SyntaxSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: m_kind(kind), m_parent(parent), m_name(name) { }

SyntaxKind SyntaxSymbol::get_kind() const
{
	return m_kind;
}

std::wstring_view SyntaxSymbol::get_name() const
{
	return gmt::resolve(m_name);
}

gmt::StringReference SyntaxSymbol::get_name_reference() const
{
	return m_name;
}

void SyntaxSymbol::set_name(std::wstring_view name)
{
	m_name = gmt::intern(name);
}

gmt::wstring SyntaxSymbol::get_full_name() const
{
	gmt::inline_vector<gmt::Ref<const SyntaxSymbol>, 8> chain;
	gmt::Ref<const SyntaxSymbol> current = m_parent;

	while (!current.is_null())
	{
		chain.push_back(current);
		current = current->get_parent();
	}

	const std::pmr::vector<gmt::Ref<const SyntaxSymbol>>& chainVector = chain.get_vector();
	std::size_t length = get_name().size() + chainVector.size();
	for (const gmt::Ref<const SyntaxSymbol>& symbol : chainVector)
		length += symbol->get_name().size();

	gmt::wstring fullName;
	fullName.reserve(length);

	bool first = true;
	for (auto it = chainVector.rbegin(); it != chainVector.rend(); ++it)
	{
		if (!first)
			fullName += L'.';

		fullName += (*it)->get_name();
		first = fullName.size() == 0;
	}

	if (!first)
		fullName += L'.';

	fullName += get_name();
	return fullName;
}

gmt::Ref<const SyntaxSymbol> SyntaxSymbol::get_parent() const
{
	return m_parent;
}

void SyntaxSymbol::on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol)
{
	// 0xDEADBEEF
}

bool SyntaxSymbol::is_type() const
{
	return false;
}

bool SyntaxSymbol::is_member() const
{
	return false;
}

bool SyntaxSymbol::is_method() const
{
	return false;
}
