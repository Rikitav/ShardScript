#pragma once
#include <memory>
#include <shard/runtime/TypeInfo.h>

using namespace std;

namespace shard::runtime
{
	struct Register
	{
	public:
		TypeInfo Type;
		shared_ptr<void> DataPtr;

		Register(TypeInfo typeInfo, shared_ptr<void> dataPtr)
			: Type(typeInfo), DataPtr(dataPtr) { }
	};
}
