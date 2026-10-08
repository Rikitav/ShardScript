#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <string_view>

namespace shard
{
	enum class SymbolAccesibility
	{
		Private,
		Public
	};

	enum class SymbolLinking
	{
		Static,
		Instance
	};

	class SHARD_API MemberSymbol : public SyntaxSymbol
	{
		SymbolLinking m_linking = SymbolLinking::Instance;
		SymbolAccesibility m_accesibility = SymbolAccesibility::Private;

	public:
		MemberSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		virtual ~MemberSymbol() = default;

		MemberSymbol(const MemberSymbol& other) = delete;
		MemberSymbol& operator=(const MemberSymbol& other) = delete;

		MemberSymbol(MemberSymbol&& other) = delete;
		MemberSymbol& operator=(MemberSymbol&& other) = delete;

		SymbolLinking get_linking() const;
		void set_linking(SymbolLinking linking);

		SymbolAccesibility get_accesibility() const;
		void set_accesibility(SymbolAccesibility accesibility);

		bool is_member() const override;
	};

	constexpr SymbolAccesibility ACS_PUBLIC = shard::SymbolAccesibility::Public;
	constexpr SymbolAccesibility ACS_PRIVATE = shard::SymbolAccesibility::Private;

	constexpr SymbolLinking LINK_STATIC = shard::SymbolLinking::Static;
	constexpr SymbolLinking LINK_INSTANCE = shard::SymbolLinking::Instance;
}
