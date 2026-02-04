#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxSymbol.h>

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
