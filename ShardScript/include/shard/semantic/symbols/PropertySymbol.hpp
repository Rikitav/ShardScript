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
		FieldSymbol* m_backingField = nullptr;
		StructSymbol* m_returnType = nullptr;
		AccessorSymbol* m_getter = nullptr;
		AccessorSymbol* m_setter = nullptr;

	public:
		PropertySymbol(std::wstring_view name, const SyntaxKind kind, SyntaxSymbol* parent);
		PropertySymbol(gmt::StringReference name, const SyntaxKind kind, SyntaxSymbol* parent);
		virtual ~PropertySymbol() = default;

		PropertySymbol(const PropertySymbol& other) = delete;
		PropertySymbol& operator=(const PropertySymbol& other) = delete;

		PropertySymbol(PropertySymbol&& other) = delete;
		PropertySymbol& operator=(PropertySymbol&& other) = delete;

		FieldSymbol* get_backing_field() const;
		void set_backing_field(FieldSymbol* backing_field);

		StructSymbol* get_return_type() const;
		void set_return_type(StructSymbol* return_type);

		AccessorSymbol* get_getter() const;
		void set_getter(AccessorSymbol* getter);

		AccessorSymbol* get_setter() const;
		void set_setter(AccessorSymbol* setter);
	};
}