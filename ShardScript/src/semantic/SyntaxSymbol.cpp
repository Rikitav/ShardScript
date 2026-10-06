#include <shard/semantic/SyntaxSymbol.hpp>

#include <string>
#include <vector>

using namespace shard;

namespace
{
	int g_definitionCounter = 0;
}

SyntaxSymbol::SyntaxSymbol(std::wstring_view name, const SyntaxKind kind)
	: m_definitionIndex(g_definitionCounter++), m_kind(kind), m_name(gmt::intern(name)) { }

int SyntaxSymbol::get_definition_index() const
{
	return m_definitionIndex;
}

SyntaxKind SyntaxSymbol::get_kind() const
{
	return m_kind;
}

std::wstring_view SyntaxSymbol::get_name() const
{
	return gmt::resolve(m_name);
}

void SyntaxSymbol::set_name(std::wstring_view name)
{
	m_name = gmt::intern(name);
}

std::wstring SyntaxSymbol::get_full_name() const
{
	std::vector<gmt::Ref<const SyntaxSymbol>> chain;
	gmt::Ref<const SyntaxSymbol> current = m_parent;

	while (!current.is_null())
	{
		chain.push_back(current);
		current = current->get_parent();
	}

	std::size_t length = get_name().size() + chain.size();
	for (const gmt::Ref<const SyntaxSymbol>& symbol : chain)
		length += symbol->get_name().size();

	std::wstring fullName;
	fullName.reserve(length);

	bool first = true;
	for (auto it = chain.rbegin(); it != chain.rend(); ++it)
	{
		if (!first)
			fullName += L'.';

		fullName += (*it)->get_name();
		first = false;
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

void SyntaxSymbol::on_symbol_declared(SyntaxSymbol* symbol)
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
