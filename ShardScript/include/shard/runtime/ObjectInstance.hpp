#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>

namespace shard
{
	class TypeShape;
	class FieldSymbol;

	class SHARD_API ObjectInstance
	{
		const TypeShape* m_shape;
		std::byte* m_memory;

		static std::uint32_t resolve_field_slot(gmt::Ref<FieldSymbol> field);

	public:
		ObjectInstance();
		ObjectInstance(const TypeShape* shape, std::byte* memory);

		static ObjectInstance null();

		bool is_null() const;
		const TypeShape* get_shape() const;
		std::byte* get_memory() const;

		ObjectInstance get_field(std::uint32_t slot) const;
		ObjectInstance get_field(gmt::Ref<FieldSymbol> field) const;
		void set_field(std::uint32_t slot, const ObjectInstance& value);
		void set_field(gmt::Ref<FieldSymbol> field, const ObjectInstance& value);

		bool as_boolean() const;
		std::int64_t as_integer() const;
		double as_double() const;
		wchar_t as_char() const;
		std::byte* as_pointer() const;

		void write_boolean(bool value);
		void write_integer(std::int64_t value);
		void write_double(double value);
		void write_char(wchar_t value);
		void write_pointer(std::byte* value);
	};
}
