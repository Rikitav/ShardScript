#pragma once
#include <cstddef>

#if defined(SHARDSCRIPT_STATIC)
	#define SHARD_API 
#else

	// Microsoft
	#if defined(_MSC_VER)
		#ifdef SHARD_API_EXPORT
			#define SHARD_API __declspec(dllexport)
		#else
			#define SHARD_API __declspec(dllimport)
		#endif
	
	// GCC / Clang
	#elif defined(__GNUC__)
		#define SHARD_API __attribute__((visibility("default")))

	// do nothing and hope for the best?
	#else
		#define SHARD_API
		#pragma warning Unknown dynamic link import/export semantics.
	#endif
#endif

// Microsoft
#if defined(_MSC_VER)

	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	using LibraryHandle = struct HINSTANCE__*;

// GCC / Clang
#elif defined(__GNUC__)

	#include <stdlib.h>
	using LibraryHandle = void*;

// Unknown
#else

	using LibraryHandle = void*;
#endif

#if _DEBUG
	#pragma warning(disable: 4251)
#endif

#if !defined(__cpp_lib_byte)
	namespace std
	{
		enum class byte : unsigned char {};
	}
#endif
