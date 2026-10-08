#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API OperatorSymbol final : public MethodSymbol
	{
		TokenType m_operatorToken;

	public:
		OperatorSymbol(std::wstring_view name, const TokenType operator_token, gmt::Ref<SyntaxSymbol> parent);
		OperatorSymbol(gmt::StringReference name, const TokenType operator_token, gmt::Ref<SyntaxSymbol> parent);
		virtual ~OperatorSymbol() = default;

		OperatorSymbol(const OperatorSymbol& other) = delete;
		OperatorSymbol& operator=(const OperatorSymbol& other) = delete;

		OperatorSymbol(OperatorSymbol&& other) = delete;
		OperatorSymbol& operator=(OperatorSymbol&& other) = delete;

		TokenType get_operator_token() const;
		void set_operator_token(const TokenType operator_token);
	};
}
