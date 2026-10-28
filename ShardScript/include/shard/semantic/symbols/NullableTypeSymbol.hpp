#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API NullableTypeSymbol final : public StructSymbol
	{
		StructSymbol* m_underlayingType = nullptr;

	public:
		NullableTypeSymbol(StructSymbol* underlaying_type, SyntaxSymbol* parent);
		virtual ~NullableTypeSymbol() = default;

		NullableTypeSymbol(const NullableTypeSymbol& other) = delete;
		NullableTypeSymbol& operator=(const NullableTypeSymbol& other) = delete;

		NullableTypeSymbol(NullableTypeSymbol&& other) = delete;
		NullableTypeSymbol& operator=(NullableTypeSymbol&& other) = delete;

		StructSymbol* get_underlaying_type() const;
	};
}