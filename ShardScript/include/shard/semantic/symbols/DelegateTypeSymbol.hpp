#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>

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
