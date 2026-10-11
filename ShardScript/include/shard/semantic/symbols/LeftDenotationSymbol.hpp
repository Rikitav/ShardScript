#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

namespace shard
{
	class StructSymbol;

	class SHARD_API LeftDenotationSymbol final : public SyntaxSymbol
	{
		gmt::Ref<StructSymbol> m_expectedType;

	public:
		LeftDenotationSymbol(gmt::Ref<StructSymbol> expected_type, gmt::Ref<SyntaxSymbol> parent);
		virtual ~LeftDenotationSymbol() = default;

		LeftDenotationSymbol(const LeftDenotationSymbol& other) = delete;
		LeftDenotationSymbol& operator=(const LeftDenotationSymbol& other) = delete;

		LeftDenotationSymbol(LeftDenotationSymbol&& other) = delete;
		LeftDenotationSymbol& operator=(LeftDenotationSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_expected_type() const;
		void set_expected_type(gmt::Ref<StructSymbol> expected_type);
	};
}
