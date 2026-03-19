#pragma once

//  Microsoft 
#if defined(_MSC_VER)
	#ifdef SHARD_API_EXPORT
		#define SHARD_API __declspec(dllexport)
	#else
		#define SHARD_API __declspec(dllimport)
	#endif

	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	using LibraryHandle = struct HINSTANCE__*;

//  GCC
#elif defined(__GNUC__)
	#define SHARD_API __attribute__((visibility("default")))
	
	#include <stdlib.h>
	using LibraryHandle = void*

//  do nothing and hope for the best?
#else
	#define SHARD_API
	#pragma warning Unknown dynamic link import/export semantics.
	
	using LibraryHandle = void*
#endif

#if _DEBUG
	#pragma warning(disable: 4251)
#endif
