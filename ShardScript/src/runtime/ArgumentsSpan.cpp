#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/symbols/ParameterSymbol.hpp>

#include <string>
#include <stdexcept>

using namespace shard;

ObjectInstance* ArgumentsSpan::operator[](int index) const
{
	return Span[index];
}

ObjectInstance* ArgumentsSpan::Find(const std::wstring& name) const
{
	int i = 0;
	for (ParameterSymbol* param : Method->Parameters)
	{
		if (param->Name == name)
			return (*this)[i];

		i += 1;
	}

	throw std::runtime_error("variable not found");
}

ArgumentsSpan::~ArgumentsSpan()
{

}
