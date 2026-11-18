#include <shard/pragmatic/TypeShapeCache.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Strings.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

using namespace shard;

namespace
{
	StructSymbol* resolve_shape_key(SymbolTable& table, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs, bool create)
	{
		if (baseType == nullptr)
			return nullptr;

		if (baseType->get_kind() == SyntaxKind::NullableType)
			baseType = static_cast<NullableTypeSymbol*>(baseType)->get_underlaying_type();

		if (baseType == nullptr)
			return nullptr;

		if (genericArgs.empty() || baseType->get_kind() == SyntaxKind::GenericType)
			return baseType;

		if (create)
			return table.find_or_create_generic_type(baseType, genericArgs);

		return table.find_generic_type(baseType, genericArgs);
	}

	std::optional<std::size_t> primitive_size(SymbolTable& table, StructSymbol* type)
	{
		if (type == table.get_boolean_primitive())
			return 1;

		if (type == table.get_byte_primitive())
			return 1;

		if (type == table.get_char_primitive())
			return sizeof(wchar_t);

		if (type == table.get_integer_primitive())
			return sizeof(std::int64_t);

		if (type == table.get_float_primitive())
			return sizeof(double);

		if (type == table.get_nint_primitive())
			return sizeof(void*);

		if (type == table.get_void_primitive())
			return 0;

		return std::nullopt;
	}

	TypeShape::ValueKind value_kind(SymbolTable& table, StructSymbol* type)
	{
		if (type == table.get_string_primitive())
			return TypeShape::ValueKind::String;

		if (type == table.get_boolean_primitive())
			return TypeShape::ValueKind::Boolean;

		if (type == table.get_byte_primitive())
			return TypeShape::ValueKind::Byte;

		if (type == table.get_char_primitive())
			return TypeShape::ValueKind::Char;

		if (type == table.get_integer_primitive())
			return TypeShape::ValueKind::Integer;

		if (type == table.get_nint_primitive())
			return TypeShape::ValueKind::NativeInteger;

		if (type == table.get_float_primitive())
			return TypeShape::ValueKind::Float;

		if (type == table.get_void_primitive())
			return TypeShape::ValueKind::Void;

		if (type->get_kind() == SyntaxKind::EnumDeclaration)
			return TypeShape::ValueKind::Enum;

		if (type->get_kind() == SyntaxKind::ArrayType)
			return TypeShape::ValueKind::Array;

		return TypeShape::ValueKind::Reference;
	}

	void reject_unsubstituted_type_parameter(StructSymbol* type)
	{
		// a type parameter has no layout of its own; reaching the layout layer
		// means some caller skipped substituting it with a concrete type argument
		if (type != nullptr && type->get_kind() == SyntaxKind::TypeParameter)
			throw std::runtime_error(std::string("cannot lay out a type shape for the unsubstituted type parameter '") + gmt::wide_to_utf8(type->get_name()).c_str() + "'");
	}
}

TypeShape* TypeShapeCache::find_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs) const
{
	StructSymbol* keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, false);
	if (keyType == nullptr)
		return nullptr;

	const auto choice = m_shapes.find(reinterpret_cast<std::uint64_t>(keyType));
	if (choice == m_shapes.end())
		return nullptr;

	return const_cast<TypeShape*>(&choice->second);
}

TypeShape* TypeShapeCache::find_or_create_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs)
{
	StructSymbol* keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, true);
	if (keyType == nullptr)
		throw std::runtime_error("cannot build a type shape for a null base type");

	reject_unsubstituted_type_parameter(keyType);

	auto [choice, inserted] = m_shapes.try_emplace(reinterpret_cast<std::uint64_t>(keyType), keyType, genericArgs);
	TypeShape& shape = choice->second;

	if (inserted)
		build(&shape, model);

	return &shape;
}

void TypeShapeCache::build(TypeShape* shape, SemanticModel& model)
{
	StructSymbol* baseType = shape->m_baseType;
	const gmt::vector<StructSymbol*>& genericArgs = shape->m_genericArguments;

	if (baseType == nullptr)
		throw std::runtime_error("cannot build a type shape for a null base type");

	SymbolTable& table = model.get_table();

	std::optional<std::size_t> baseTypePrimitiveSize = primitive_size(table, baseType);
	bool isEnum = baseType->get_kind() == SyntaxKind::EnumDeclaration;
	shape->m_isReference = !baseTypePrimitiveSize.has_value() && !isEnum;
	shape->m_valueKind = value_kind(table, baseType);

	if (baseType->get_kind() == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(baseType);
		TypeShape* elementShape = find_or_create_type_shape(model, arrayType->get_underlaying_type());

		shape->m_elementShape = elementShape;
		shape->m_size = sizeof(std::int64_t) + sizeof(void*) * arrayType->get_length();
		return;
	}

	if (baseType->get_kind() == SyntaxKind::EnumDeclaration)
	{
		shape->m_size = sizeof(std::int64_t);
		return;
	}

	GenericTypeSymbol* within = nullptr;
	StructSymbol* fieldOwner = baseType;

	if (baseType->get_kind() == SyntaxKind::GenericType)
	{
		within = static_cast<GenericTypeSymbol*>(baseType);
		fieldOwner = within->get_underlaying_type();
	}
	else if (!baseType->type_parameters.empty() && !genericArgs.empty())
	{
		within = table.find_or_create_generic_type(baseType, genericArgs);
	}

	std::size_t offset = 0;
	for (FieldSymbol* field : fieldOwner->fields)
	{
		if ((field == nullptr) || (field->get_return_type() == nullptr))
			continue;

		if (field->get_linking() == LINK_STATIC)
			continue;

		StructSymbol* fieldType = field->get_return_type();
		if (within != nullptr)
			fieldType = model.substitute_type(fieldType, within);

		if (fieldType == nullptr)
			continue;

		TypeShape::SlotInfo slot{};
		slot.offset = offset;
		slot.isNullable = fieldType->get_kind() == SyntaxKind::NullableType;

		slot.fieldShape = find_or_create_type_shape(model, fieldType);

		offset += sizeof(void*);
		shape->m_slots.push_back(slot);
	}

	if (offset == 0)
		shape->m_size = baseTypePrimitiveSize.has_value() ? baseTypePrimitiveSize.value() : offset;
	else
		shape->m_size = offset;

	// a fieldless struct occupies one byte by convention: an empty payload has no
	// address to hand out, and in-place frame storage is gated on size > 0
	if (offset == 0 && fieldOwner->get_kind() == SyntaxKind::StructDeclaration)
	{
		const bool builtinStorage = model.is_primitive_type(baseType)
			|| baseType == table.get_void_primitive()
			|| baseType == table.get_null_primitive();

		if (!builtinStorage)
			shape->m_size = 1;
	}
}
