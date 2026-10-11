#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>
#include <gmt/Interner.hpp>

#include <cmath>
#include <cstdint>

namespace shard
{
	class StructSymbol;

	class SHARD_API LiteralSymbol final : public SyntaxSymbol
	{
		union
		{
			bool m_asBoolean = false;
			std::int64_t m_asInteger;
			std::double_t m_asDouble;
			wchar_t m_asChar;
			gmt::StringReference m_asString;
		};

		const TokenType m_literalType;
		gmt::Ref<StructSymbol> m_boundType;

	public:
		LiteralSymbol(const TokenType literal_type, gmt::Ref<SyntaxSymbol> parent);
		virtual ~LiteralSymbol() = default;

		LiteralSymbol(const LiteralSymbol& other) = delete;
		LiteralSymbol& operator=(const LiteralSymbol& other) = delete;

		LiteralSymbol(LiteralSymbol&& other) = delete;
		LiteralSymbol& operator=(LiteralSymbol&& other) = delete;

		TokenType get_literal_type() const;

		bool get_boolean_value() const;
		void set_boolean_value(bool value);

		std::int64_t get_integer_value() const;
		void set_integer_value(std::int64_t value);

		std::double_t get_double_value() const;
		void set_double_value(std::double_t value);

		wchar_t get_char_value() const;
		void set_char_value(wchar_t value);

		gmt::StringReference get_string_value() const;
		void set_string_value(gmt::StringReference value);

		gmt::Ref<StructSymbol> get_bound_type() const;
		void set_bound_type(gmt::Ref<StructSymbol> bound_type);
	};
}
