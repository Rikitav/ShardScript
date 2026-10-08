#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <string_view>

namespace shard
{
	class StructSymbol;
	class FieldSymbol;
	class AccessorSymbol;
	class ExpressionSyntax;

	class SHARD_API PropertySymbol : public MemberSymbol
	{
		gmt::Ref<FieldSymbol> m_backingField;
		gmt::Ref<StructSymbol> m_returnType;
		gmt::Ref<AccessorSymbol> m_getter;
		gmt::Ref<AccessorSymbol> m_setter;

	public:
		PropertySymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		PropertySymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		virtual ~PropertySymbol() = default;

		PropertySymbol(const PropertySymbol& other) = delete;
		PropertySymbol& operator=(const PropertySymbol& other) = delete;

		PropertySymbol(PropertySymbol&& other) = delete;
		PropertySymbol& operator=(PropertySymbol&& other) = delete;

		gmt::Ref<FieldSymbol> get_backing_field() const;
		void set_backing_field(gmt::Ref<FieldSymbol> backing_field);

		gmt::Ref<StructSymbol> get_return_type() const;
		void set_return_type(gmt::Ref<StructSymbol> return_type);

		gmt::Ref<AccessorSymbol> get_getter() const;
		void set_getter(gmt::Ref<AccessorSymbol> getter);

		gmt::Ref<AccessorSymbol> get_setter() const;
		void set_setter(gmt::Ref<AccessorSymbol> setter);
	};
}
