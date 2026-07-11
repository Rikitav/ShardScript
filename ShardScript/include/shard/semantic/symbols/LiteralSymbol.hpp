#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

namespace shard
{
	class SHARD_API LiteralSymbol : public SyntaxSymbol
	{
	public:
		union
		{
			bool AsBooleanValue = false;
			std::int64_t AsIntegerValue;
			double AsDoubleValue;
			wchar_t AsCharValue;
		};

		const TokenType LiteralType;
		TypeSymbol* BoundType = nullptr;

		LiteralSymbol(const TokenType type)
			: SyntaxSymbol(L"Literal", SyntaxKind::LiteralExpression), LiteralType(type) { }
	};
}
