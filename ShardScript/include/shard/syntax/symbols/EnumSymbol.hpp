#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <string>

namespace shard
{
	class SHARD_API EnumSymbol : public TypeSymbol
	{
	public:
		bool IsFlags = false;

		inline EnumSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::EnumDeclaration)
		{
			Inlining = TypeInlining::ByValue;
			MemoryBytesSize = sizeof(std::int64_t);
			State = TypeLayoutingState::Visited;
		}

		inline EnumSymbol(const EnumSymbol& other) = delete;

		inline virtual ~EnumSymbol() override = default;
	};
}
