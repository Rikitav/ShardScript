#include <shard/compilation/VirtualProgramImage.hpp>

using namespace shard;

BytecodeSection& VirtualProgramImage::get_text_section()
{
	return m_textSection;
}

const BytecodeSection& VirtualProgramImage::get_text_section() const
{
	return m_textSection;
}

DataSection& VirtualProgramImage::get_data_section()
{
	return m_dataSection;
}

const DataSection& VirtualProgramImage::get_data_section() const
{
	return m_dataSection;
}

std::uint32_t VirtualProgramImage::add_string_literal(std::size_t offset, std::size_t length)
{
	const std::uint32_t index = static_cast<std::uint32_t>(m_stringLiterals.size());
	m_stringLiterals.push_back(StringLiteral{ offset, length });
	return index;
}

const VirtualProgramImage::StringLiteral& VirtualProgramImage::get_string_literal(std::uint32_t index) const
{
	return m_stringLiterals.at(index);
}

std::size_t VirtualProgramImage::get_string_literal_count() const
{
	return m_stringLiterals.size();
}

void VirtualProgramImage::set_entry_point(MethodSymbol* method)
{
	m_entryPoint = method;
}

MethodSymbol* VirtualProgramImage::get_entry_point() const
{
	return m_entryPoint;
}