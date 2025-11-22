#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class ClassSymbol : public TypeSymbol
	{
	public:
		std::vector<MethodSymbol*> Constructors;

		inline ClassSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::ClassDeclaration)
		{
			//MemoryBytesSize += sizeof(unsigned long);
			IsReferenceType = true;
		}

		inline ~ClassSymbol() override
		{
			for (MethodSymbol* ctor : Constructors)
				delete ctor;
		}
	};
}
