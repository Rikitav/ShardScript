#include <shard/pragmatic/TypeShapeCache.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Containers.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace shard;

namespace
{
	StructSymbol* resolve_shape_key(SymbolTable& table, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs, bool create)
	{
		if ((baseType == nullptr))
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

		if (type == table.get_double_primitive())
			return sizeof(double);

		if (type == table.get_nint_primitive())
			return sizeof(void*);

		if (type == table.get_void_primitive())
			return 0;

		return std::nullopt;
	}
}

TypeShape* TypeShapeCache::find_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs) const
{
	StructSymbol* keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, false);
	if ((keyType == nullptr))
		return nullptr;

	const auto choice = m_shapes.find(reinterpret_cast<std::uint64_t>(keyType));
	if (choice == m_shapes.end())
		return nullptr;

	return const_cast<TypeShape*>(&choice->second);
}

TypeShape* TypeShapeCache::find_or_create_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs)
{
	StructSymbol* keyType = resolve_shape_key(model.get_table(), baseType, genericArgs, true);
	if ((keyType == nullptr))
		throw std::runtime_error("cannot build a type shape for a null base type");

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

	if ((baseType == nullptr))
		throw std::runtime_error("cannot build a type shape for a null base type");

	SymbolTable& table = model.get_table();

	// everything that is not a sized primitive or an enum is reference-stored;
	// FrameLayout and the future GC maps consume this flag
	std::optional<std::size_t> baseTypePrimitiveSize = primitive_size(table, baseType);
	bool isEnum = baseType->get_kind() == SyntaxKind::EnumDeclaration;
	shape->m_isReference = !baseTypePrimitiveSize.has_value() && !isEnum;

	if (baseType->get_kind() == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(baseType);
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

		// substitution only matters for raw type parameters
		StructSymbol* fieldType = field->get_return_type();
		if (!(within == nullptr) && fieldType->get_kind() == SyntaxKind::TypeParameter)
			fieldType = within->substitute_type_parameter(static_cast<TypeParameterSymbol*>(fieldType));

		if ((fieldType == nullptr))
			continue;

		TypeShape::SlotInfo slot{};
		slot.offset = offset;
		slot.fieldShape = find_or_create_type_shape(model, fieldType);

		std::optional<std::size_t> fieldTypeSize = primitive_size(table, fieldType);
		offset += fieldTypeSize.has_value() ? fieldTypeSize.value() : sizeof(void*);

		shape->m_slots.push_back(slot);
	}

	// a fieldless primitive keeps its intrinsic size; composites take the laid-out size
	if (offset == 0)
		shape->m_size = baseTypePrimitiveSize.has_value() ? baseTypePrimitiveSize.value() : offset;
	else
		shape->m_size = offset;
}
