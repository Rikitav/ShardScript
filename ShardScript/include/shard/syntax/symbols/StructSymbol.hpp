#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API StructSymbol : public TypeSymbol
	{
	public:
		inline StructSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::StructDeclaration)
		{
			Inlining = TypeInlining::ByValue;
		}

		inline StructSymbol(const StructSymbol& other) = delete;

		inline virtual ~StructSymbol() = default;
	};
}
