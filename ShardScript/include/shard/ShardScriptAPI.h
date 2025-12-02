//#if _DEBUG
#pragma warning(disable: 4251)
//#endif

#ifdef SHARD_API_EXPORT
	#define SHARD_API __declspec(dllexport)
#else
	#define SHARD_API __declspec(dllimport)
#endif