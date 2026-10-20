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

void VirtualProgramImage::register_method(gmt::Ref<MethodSymbol> method)
{
	m_methods.push_back(method);
}

const std::vector<gmt::Ref<MethodSymbol>>& VirtualProgramImage::get_methods() const
{
	return m_methods;
}

void VirtualProgramImage::set_entry_point(gmt::Ref<MethodSymbol> method)
{
	m_entryPoint = method;
}

gmt::Ref<MethodSymbol> VirtualProgramImage::get_entry_point() const
{
	return m_entryPoint;
}
