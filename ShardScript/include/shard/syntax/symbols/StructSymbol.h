#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/SyntaxKind.h>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API StructSymbol : public TypeSymbol
	{
	public:
		//std::vector<MethodSymbol*> Constructors;

		inline StructSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::StructDeclaration)
		{
			//MemoryBytesSize += 0;
			IsValueType = true;
		}

		inline StructSymbol(const StructSymbol& other) = delete;

		inline virtual ~StructSymbol() override
		{
			/*
			for (MethodSymbol* ctor : Constructors)
				delete ctor;
			*/
		}
	};
}
