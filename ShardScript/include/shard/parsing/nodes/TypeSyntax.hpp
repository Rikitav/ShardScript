#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API TypeSyntax : public SyntaxNode
	{
	public:
		shard::TypeSymbol* Symbol = nullptr;

		inline TypeSyntax(const SyntaxKind kind, SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline TypeSyntax(const TypeSyntax& other) = delete;

		inline virtual ~TypeSyntax() { }

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}