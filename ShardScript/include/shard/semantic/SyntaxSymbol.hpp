#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <gmt/Interner.hpp>
#include <gmt/Arena.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API SyntaxSymbol
	{
		SyntaxKind m_kind;
		gmt::Ref<SyntaxSymbol> m_parent;
		gmt::StringReference m_name;

	public:
		SyntaxSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		virtual ~SyntaxSymbol() = default;

		SyntaxSymbol(const SyntaxSymbol& other) = delete;
		SyntaxSymbol& operator=(const SyntaxSymbol& other) = delete;

		SyntaxSymbol(SyntaxSymbol&& other) = delete;
		SyntaxSymbol& operator=(SyntaxSymbol&& other) = delete;

		SyntaxKind get_kind() const;

		std::wstring_view get_name() const;
		void set_name(std::wstring_view name);

		std::wstring get_full_name() const;
		gmt::Ref<const SyntaxSymbol> get_parent() const;

		virtual void on_symbol_declared(SyntaxSymbol* symbol);
		virtual bool is_type() const;
		virtual bool is_member() const;
		virtual bool is_method() const;
	};
}
