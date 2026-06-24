#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API OperatorSymbol : public MethodSymbol
	{
	public:
		TokenType OperatorToken;

		inline OperatorSymbol(std::wstring name, TokenType opToken)
			: MethodSymbol(name, SyntaxKind::OperatorDeclaration), OperatorToken(opToken) { }

		inline OperatorSymbol(std::wstring name, TokenType opToken, MethodSymbolDelegate delegate)
			: MethodSymbol(name, SyntaxKind::OperatorDeclaration), OperatorToken(opToken)
		{
			FunctionPointer = delegate;
			HandleType = MethodHandleType::External;
		}

		inline OperatorSymbol(const OperatorSymbol& other) = delete;

		inline virtual ~OperatorSymbol() = default;
	};
}
