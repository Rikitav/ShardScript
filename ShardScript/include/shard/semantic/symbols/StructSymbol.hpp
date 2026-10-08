#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API StructSymbol : public MemberSymbol
	{
	public:
		StructSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		virtual ~StructSymbol() = default;

		StructSymbol(const StructSymbol& other) = delete;
		StructSymbol& operator=(const StructSymbol& other) = delete;

		StructSymbol(StructSymbol&& other) = delete;
		StructSymbol& operator=(StructSymbol&& other) = delete;

		bool is_type() const override;
	};
}
