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
		gmt::Ref<StructSymbol> m_underlayingType;

	public:
		NullableTypeSymbol(gmt::Ref<StructSymbol> underlaying_type, gmt::Ref<SyntaxSymbol> parent);
		virtual ~NullableTypeSymbol() = default;

		NullableTypeSymbol(const NullableTypeSymbol& other) = delete;
		NullableTypeSymbol& operator=(const NullableTypeSymbol& other) = delete;

		NullableTypeSymbol(NullableTypeSymbol&& other) = delete;
		NullableTypeSymbol& operator=(NullableTypeSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_underlaying_type() const;
	};
}