#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/SyntaxKind.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class SHARD_API ClassSymbol : public TypeSymbol
	{
	public:
		std::vector<MethodSymbol*> Constructors;

		inline ClassSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::ClassDeclaration)
		{
			//MemoryBytesSize += sizeof(unsigned long);
			IsReferenceType = true;
		}

		inline ClassSymbol(const ClassSymbol& other) = delete;

		inline virtual ~ClassSymbol() override
		{
			for (MethodSymbol* ctor : Constructors)
				delete ctor;
		}
	};
}
