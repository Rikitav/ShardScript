#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API TypeSyntax : public SyntaxNode
	{
	public:
		shard::TypeSymbol* Symbol = nullptr;

		inline TypeSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline TypeSyntax(const TypeSyntax& other) = delete;

		inline virtual ~TypeSyntax() { }

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}