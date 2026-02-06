#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <string>
#include <stdexcept>

using namespace shard;

const ObjectInstance* ArgumentsSpan::operator[](int index)
{
	const ObjectInstance* instance = Start + index;
	return instance;
}

const ObjectInstance* ArgumentsSpan::Find(const std::wstring& name)
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
