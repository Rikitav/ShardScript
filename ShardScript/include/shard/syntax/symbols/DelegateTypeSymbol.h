#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <new>
#include <vector>

namespace shard::syntax::symbols
{
	class DelegateTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* ReturnType = nullptr;
		std::vector<ParameterSymbol*> Parameters;
		MethodSymbol* AnonymousSymbol = nullptr;

		inline DelegateTypeSymbol() : TypeSymbol(L"Delegate", SyntaxKind::DelegateType)
		{
			IsReferenceType = true;
		}

		inline DelegateTypeSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::DelegateType)
		{
			IsReferenceType = true;
		}

		inline ~DelegateTypeSymbol()
		{
			for (ParameterSymbol* parameter : Parameters)
				delete parameter;
		}
	};
}
