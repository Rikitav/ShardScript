#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <cstdint>
#include <string_view>

namespace shard
{
	class StructSymbol;

	class SHARD_API FieldSymbol final : public MemberSymbol
	{
		gmt::Ref<StructSymbol> m_returnType;
		bool m_isEnumValue = false;
		std::int64_t m_enumValue = 0;

	public:
		FieldSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		FieldSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~FieldSymbol() = default;

		FieldSymbol(const FieldSymbol& other) = delete;
		FieldSymbol& operator=(const FieldSymbol& other) = delete;

		FieldSymbol(FieldSymbol&& other) = delete;
		FieldSymbol& operator=(FieldSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_return_type() const;
		void set_return_type(gmt::Ref<StructSymbol> return_type);

		bool get_is_enum_value() const;
		void set_is_enum_value(bool is_enum_value);

		std::int64_t get_enum_value() const;
		void set_enum_value(std::int64_t enum_value);
	};
}
