#pragma once
#include <memory>

using namespace std;

namespace shard::runtime
{
	static const int TYPE_CODE_BOOLEAN = 1;
	static const int TYPE_CODE_INTEGER = 2;
	static const int TYPE_CODE_STRING = 3;

	struct Register
	{
	public:
		size_t Index;
		int TypeCode;
		shared_ptr<void> DataPtr;

		Register(size_t index)
			: Index(index), TypeCode(-1), DataPtr(nullptr) { }

		Register(int typeCode, shared_ptr<void> dataPtr)
			: Index(-1), TypeCode(typeCode), DataPtr(dataPtr) { }
	};
}
