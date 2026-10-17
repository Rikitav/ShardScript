#include <shard/pragmatic/TypeShapeCache.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace shard;

namespace
{
	constexpr std::size_t NotAPrimitive = std::numeric_limits<std::size_t>::max();

	gmt::Ref<StructSymbol> resolve_shape_key(SymbolTable& table, gmt::Ref<StructSymbol> baseType, const std::vector<gmt::Ref<StructSymbol>>& genericArgs, bool create)
	{
		if (baseType.is_null())
			return gmt::nullref;

		if (genericArgs.empty() || baseType->get_kind() == SyntaxKind::GenericType)
			return baseType;

		if (create)
			return table.find_or_create_generic_type(baseType, genericArgs);

		return table.find_generic_type(baseType, genericArgs);
	}

	std::size_t primitive_size(SymbolTable& table, gmt::Ref<StructSymbol> type)
	{
		if (type == table.get_boolean_primitive())
			return 1;

		if (type == table.get_byte_primitive())
			return 1;

		if (type == table.get_char_primitive())
			return sizeof(wchar_t);

		if (type == table.get_integer_primitive())
			return sizeof(std::int64_t);

		if (type == table.get_double_primitive())
			return sizeof(double);

		if (type == table.get_nint_primitive())
			return sizeof(void*);

		if (type == table.get_void_primitive())
			return 0;

		return NotAPrimitive;
	}
}

TypeShape* TypeShapeCache::find_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, const std::vector<gmt::Ref<StructSymbol>>& genericArgs) const
{
	gmt::Ref<StructSymbol> keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, false);
	if (keyType.is_null())
		return nullptr;

	const auto choice = m_shapes.find(keyType.get_offset());
	if (choice == m_shapes.end())
		return nullptr;

	return const_cast<TypeShape*>(&choice->second);
}

TypeShape* TypeShapeCache::find_or_create_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, const std::vector<gmt::Ref<StructSymbol>>& genericArgs)
{
	gmt::Ref<StructSymbol> keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, true);
	if (keyType.is_null())
		throw std::runtime_error("cannot build a type shape for a null base type");

	auto [choice, inserted] = m_shapes.try_emplace(keyType.get_offset(), keyType, genericArgs);
	TypeShape& shape = choice->second;

	if (inserted)
		build(&shape, model);

	return &shape;
}

void TypeShapeCache::build(TypeShape* shape, SemanticModel& model)
{
	gmt::Ref<StructSymbol> baseType = shape->m_baseType;
	const std::vector<gmt::Ref<StructSymbol>>& genericArgs = shape->m_genericArguments;

	if (baseType.is_null())
		throw std::runtime_error("cannot build a type shape for a null base type");

	SymbolTable& table = model.get_table();

	// everything that is not a sized primitive or an enum is reference-stored;
	// FrameLayout and the future GC maps consume this flag
	bool isPrimitive = primitive_size(table, baseType) != NotAPrimitive;
	bool isEnum = baseType->get_kind() == SyntaxKind::EnumDeclaration;
	shape->m_isReference = !isPrimitive && !isEnum;

	if (baseType->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<ArrayTypeSymbol> arrayType = baseType.as<ArrayTypeSymbol>();
		TypeShape* elementShape = find_or_create_type_shape(model, arrayType->get_underlaying_type());

		shape->m_size = sizeof(std::int64_t) + elementShape->get_size() * arrayType->get_length();
		return;
	}

	if (baseType->get_kind() == SyntaxKind::EnumDeclaration)
	{
		shape->m_size = sizeof(std::int64_t);
		return;
	}

	// the substitution context for fields: a queried generic instance uses
	// itself, a generic definition with provided arguments gets one created
	gmt::Ref<GenericTypeSymbol> within = gmt::nullref;
	gmt::Ref<StructSymbol> fieldOwner = baseType;

	if (baseType->get_kind() == SyntaxKind::GenericType)
	{
		within = baseType.as<GenericTypeSymbol>();
		fieldOwner = within->get_underlaying_type();
	}
	else if (!baseType->type_parameters.empty() && !genericArgs.empty())
	{
		within = table.find_or_create_generic_type(baseType, genericArgs);
	}

	std::size_t offset = 0;
	for (gmt::Ref<FieldSymbol> field : fieldOwner->fields)
	{
		if (field.is_null() || field->get_return_type().is_null())
			continue;

		if (field->get_linking() == LINK_STATIC)
			continue;

		// substitution only matters for raw type parameters
		gmt::Ref<StructSymbol> fieldType = field->get_return_type();
		if (!within.is_null() && fieldType->get_kind() == SyntaxKind::TypeParameter)
			fieldType = within->substitute_type_parameter(fieldType.as<TypeParameterSymbol>());

		if (fieldType.is_null())
			continue;

		TypeShape::SlotInfo slot{};
		slot.offset = offset;
		slot.fieldShape = find_or_create_type_shape(model, fieldType);

		std::size_t size = primitive_size(table, fieldType);
		offset += size != NotAPrimitive ? size : sizeof(void*);

		shape->m_slots.push_back(slot);
	}

	// a fieldless primitive keeps its intrinsic size
	std::size_t basePrimitive = primitive_size(table, baseType);
	if (offset == 0 && basePrimitive != NotAPrimitive)
		shape->m_size = basePrimitive;
	else
		shape->m_size = offset;
}
