#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/SyntaxKind.h>

#include <new>
#include <vector>

namespace shard
{
	class SHARD_API DelegateTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* ReturnType = nullptr;
		std::vector<ParameterSymbol*> Parameters;
		MethodSymbol* AnonymousSymbol = nullptr;

		inline DelegateTypeSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::DelegateType)
		{
			IsReferenceType = true;
		}

		inline DelegateTypeSymbol(const DelegateTypeSymbol& other) = delete;

		inline virtual ~DelegateTypeSymbol()
		{
			if (AnonymousSymbol != nullptr)
				delete AnonymousSymbol;

			for (ParameterSymbol* parameter : Parameters)
				delete parameter;
		}
	};
}
