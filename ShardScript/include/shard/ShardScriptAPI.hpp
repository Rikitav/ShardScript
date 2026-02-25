#pragma once

#if defined(_WIN32) || defined(_WIN64)
	// Windows	
	#ifdef SHARD_API_EXPORT
		#define SHARD_API __declspec(dllexport)
	#else
		#define SHARD_API __declspec(dllimport)
	#endif
#else
	// Linux / Mac
	#define SHARD_API __attribute__((visibility("default")))
#endif

#if _DEBUG
	#pragma warning(disable: 4251)
#endif

#ifdef _WIN32
	using LibraryHandle = struct HINSTANCE__*;
#else
	using LibraryHandle = void*
#endif
