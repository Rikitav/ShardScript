#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <gmt/Interner.hpp>
#include <gmt/Arena.hpp>

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

	class SHARD_API SyntaxSymbol
	{
		int m_definitionIndex;
		SyntaxKind m_kind;
		gmt::Ref<SyntaxSymbol> m_parent;
		rs::stringintern::StringReference m_name;

		SymbolAccesibility m_accesibility = SymbolAccesibility::Private;

	public:
		SyntaxSymbol(std::wstring_view name, const SyntaxKind kind);
		virtual ~SyntaxSymbol() = default;

		SyntaxSymbol(const SyntaxSymbol& other) = delete;
		SyntaxSymbol& operator=(const SyntaxSymbol& other) = delete;

		SyntaxSymbol(SyntaxSymbol&& other) = delete;
		SyntaxSymbol& operator=(SyntaxSymbol&& other) = delete;

		int get_definition_index() const;
		SyntaxKind get_kind() const;

		std::wstring_view get_name() const;
		void set_name(std::wstring_view name);

		std::wstring get_full_name() const;
		gmt::Ref<const SyntaxSymbol> get_parent() const;

		SymbolAccesibility get_accesibility() const;
		void set_accesibility(SymbolAccesibility accesibility);

		virtual void on_symbol_declared(SyntaxSymbol* symbol);
		virtual bool is_type() const;
		virtual bool is_member() const;
		virtual bool is_method() const;
	};

	constexpr SymbolLinking LINK_STATIC = shard::SymbolLinking::Static;
	constexpr SymbolLinking LINK_INSTANCE = shard::SymbolLinking::Instance;

	constexpr shard::SymbolAccesibility ACS_PUBLIC = shard::SymbolAccesibility::Public;
	constexpr shard::SymbolAccesibility ACS_PRIVATE = shard::SymbolAccesibility::Private;
}
