#pragma once

namespace shard::runtime
{
	struct TypeInfo
	{
		int Id;
		bool IsRef;

		TypeInfo(int id, bool isRef)
			: Id(id), IsRef(isRef) {
		}
	};

	static const int TYPE_CODE_BOOLEAN = 1;
	static const int TYPE_CODE_INTEGER = 2;
	static const int TYPE_CODE_STRING = 3;

	static const TypeInfo TYPEINFO_BOOLEAN = TypeInfo(TYPE_CODE_BOOLEAN, false);
	static const TypeInfo TYPEINFO_INTEGER = TypeInfo(TYPE_CODE_INTEGER, false);
	static const TypeInfo TYPEINFO_STRING = TypeInfo(TYPE_CODE_STRING, true);
}
