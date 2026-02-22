#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>

namespace shard
{
	class SHARD_API LiteralSymbol : public SyntaxSymbol
	{
	public:
		union
		{
			bool AsBooleanValue = false;
			int64_t AsIntegerValue;
			double AsDoubleValue;
		};

		const TokenType LiteralType;

		LiteralSymbol(const TokenType type)
			: SyntaxSymbol(L"Literal", SyntaxKind::LiteralExpression), LiteralType(type) { }
	};
}
