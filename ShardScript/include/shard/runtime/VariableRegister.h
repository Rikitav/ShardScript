#pragma once
#include <shard/runtime/TypeInfo.h>
#include <memory>

using namespace std;

namespace shard::runtime
{
	struct VariableRegister
	{
	public:
		shard::runtime::TypeInfo Type;
		std::shared_ptr<void> DataPtr;

		VariableRegister(shard::runtime::TypeInfo typeInfo, std::shared_ptr<void> dataPtr)
			: Type(typeInfo), DataPtr(dataPtr) { }
	};
}
