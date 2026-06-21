#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <new>
#include <vector>
#include <memory>

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
			Inlining = TypeInlining::ByReference;
		}

		inline DelegateTypeSymbol(const DelegateTypeSymbol& other) = delete;

		inline virtual ~DelegateTypeSymbol() = default;
	};
}
