#include <shard/compilation/VirtualProgramImage.hpp>

using namespace shard;

std::vector<std::byte>& VirtualProgramImage::get_text_section()
{
	return m_textSection;
}

const std::vector<std::byte>& VirtualProgramImage::get_text_section() const
{
	return m_textSection;
}

std::vector<std::byte>& VirtualProgramImage::get_data_section()
{
	return m_dataSection;
}

const std::vector<std::byte>& VirtualProgramImage::get_data_section() const
{
	return m_dataSection;
}

std::uint32_t VirtualProgramImage::intern_symbol(gmt::Ref<SyntaxSymbol> symbol)
{
	const std::size_t key = symbol.get_offset();
	const auto existing = m_symbolHandles.find(key);
	if (existing != m_symbolHandles.end())
		return existing->second;

	const std::uint32_t handle = static_cast<std::uint32_t>(m_symbols.size());
	m_symbols.push_back(symbol);
	m_symbolHandles.emplace(key, handle);
	return handle;
}

gmt::Ref<SyntaxSymbol> VirtualProgramImage::resolve_symbol(std::uint32_t handle) const
{
	if (handle >= m_symbols.size())
		return gmt::nullref;

	return m_symbols[handle];
}

const std::vector<gmt::Ref<SyntaxSymbol>>& VirtualProgramImage::get_symbols() const
{
	return m_symbols;
}

void VirtualProgramImage::set_entry_point(gmt::Ref<MethodSymbol> method)
{
	m_entryPoint = method;
}

gmt::Ref<MethodSymbol> VirtualProgramImage::get_entry_point() const
{
	return m_entryPoint;
}
