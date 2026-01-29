#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard
{
	class SHARD_API LiteralExpressionSyntax : public ExpressionSyntax
	{
	public:
		enum ValueType
		{
			AsNull,
			AsBoolean,
			AsInteger,
			AsDouble,
			AsChar,
			AsString,
		};

		union
		{
			bool AsBooleanValue = false;
			int64_t AsIntegerValue;
			double AsDoubleValue;
			wchar_t AsCharValue;
			std::wstring* AsStringValue;
		};

		const SyntaxToken LiteralToken;
		ValueType Type = AsNull;

		inline LiteralExpressionSyntax(const SyntaxToken literal, const SyntaxNode* parent) : ExpressionSyntax(SyntaxKind::LiteralExpression, parent), LiteralToken(literal) { }
		inline LiteralExpressionSyntax(const LiteralExpressionSyntax&) = delete;
		inline virtual ~LiteralExpressionSyntax() { }
	};
}
