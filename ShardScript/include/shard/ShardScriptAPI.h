#pragma once

#ifdef SHARD_API_EXPORT
	#define SHARD_API __declspec(dllexport)
#else
	#define SHARD_API __declspec(dllimport)
#endif

#if _DEBUG
#pragma warning(disable: 4251)
#endif

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define SafeInit TOKENPASTE2(InitLibrary_, _MSC_VER)

extern "C" __declspec(dllexport) void SafeInit();

//#define EXPORT_EXTERN(name) ObjectInstance* &name(MethodSymbol* method, InboundVariableContext* arguments)
