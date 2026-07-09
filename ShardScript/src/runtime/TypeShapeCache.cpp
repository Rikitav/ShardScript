#include <shard/runtime/TypeShapeCache.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <stdexcept>
#include <algorithm>
#include <limits>

using namespace shard;

std::size_t TypeShapeCache::KeyHash::operator()(const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& key) const
{
	std::size_t hash = std::hash<TypeSymbol*>{}(key.first);
	for (TypeSymbol* argument : key.second)
	{
		hash ^= std::hash<TypeSymbol*>{}(argument) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}

	return hash;
}

bool TypeShapeCache::KeyEquals::operator()(const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& lhs,
	const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& rhs) const
{
	if (lhs.first != rhs.first)
		return false;

	if (lhs.second.size() != rhs.second.size())
		return false;

	for (std::size_t i = 0; i < lhs.second.size(); ++i)
	{
		if (lhs.second[i] != rhs.second[i])
			return false;
	}

	return true;
}

TypeSymbol* TypeShapeCache::SubstituteTypeParameter(TypeSymbol* type,
	const std::vector<TypeParameterSymbol*>& parameters,
	const std::vector<TypeSymbol*>& arguments)
{
	if (type == nullptr)
		return nullptr;

	if (type->Kind == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(type);
		for (std::size_t i = 0; i < parameters.size(); ++i)
		{
			if (parameters[i] == typeParameter)
				return i < arguments.size() ? arguments[i] : type;
		}

		return type;
	}

	if (type->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
		TypeSymbol* substitutedElement = SubstituteTypeParameter(arrayType->UnderlayingType, parameters, arguments);
		if (substitutedElement == nullptr || substitutedElement == arrayType->UnderlayingType)
			return type;

		ArrayTypeSymbol* substitutedArray = new ArrayTypeSymbol(substitutedElement);
		substitutedArray->Length = arrayType->Length;
		substitutedArray->MemoryBytesSize = arrayType->MemoryBytesSize;
		substitutedArray->LayoutingState = arrayType->LayoutingState;
		return substitutedArray;
	}

	return type;
}

TypeShape* TypeShapeCache::GetShape(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs) const
{
	auto key = std::make_pair(baseType, genericArgs);
	auto it = _shapes.find(key);
	if (it != _shapes.end())
		return it->second.get();

	return nullptr;
}

TypeShape* TypeShapeCache::GetOrCreateShape(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs)
{
	auto key = std::make_pair(baseType, genericArgs);
	auto it = _shapes.find(key);
	if (it != _shapes.end())
		return it->second.get();

	std::unique_ptr<TypeShape> shape = std::make_unique<TypeShape>(baseType, genericArgs);
	TypeShape* shapePtr = shape.get();
	_shapes.emplace(key, std::move(shape));

	BuildShape(shapePtr, baseType, genericArgs);
	return shapePtr;
}

void TypeShapeCache::BuildShape(TypeShape* shape, TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs)
{
	if (baseType == nullptr)
		throw std::runtime_error("Cannot build TypeShape for null base type");

	if (baseType->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(baseType);
		TypeShape* elementShape = GetOrCreateShape(arrayType->UnderlayingType);
		shape->Size = SymbolTable::Primitives::Array->MemoryBytesSize + elementShape->Size * arrayType->Length;
		return;
	}

	std::vector<TypeParameterSymbol*> parameters;
	std::vector<TypeSymbol*> arguments;
	if (baseType->TypeParameters.size() > 0 && genericArgs.size() > 0)
	{
		parameters.reserve(baseType->TypeParameters.size());
		for (TypeParameterSymbol* parameter : baseType->TypeParameters)
			parameters.push_back(parameter);

		arguments = genericArgs;
	}

	std::uint32_t maxSlotIndex = 0;
	bool hasSlots = false;
	for (FieldSymbol* field : baseType->Fields)
	{
		if (field->Linking == LINK_STATIC)
			continue;

		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			continue;

		maxSlotIndex = std::max(maxSlotIndex, field->SlotIndex);
		hasSlots = true;
	}

	if (hasSlots)
		shape->Slots.resize(static_cast<std::size_t>(maxSlotIndex) + 1);

	std::size_t offset = 0;
	for (FieldSymbol* field : baseType->Fields)
	{
		if (field->Linking == LINK_STATIC)
			continue;

		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			continue;

		TypeSymbol* fieldType = field->ReturnType;
		if (fieldType == nullptr)
			continue;

		TypeSymbol* concreteFieldType = SubstituteTypeParameter(fieldType, parameters, arguments);
		if (concreteFieldType == nullptr)
			concreteFieldType = fieldType;

		TypeShape::SlotInfo slot;
		slot.Offset = offset;

		if (concreteFieldType->Kind == SyntaxKind::ArrayType)
		{
			slot.FieldShape = GetOrCreateShape(concreteFieldType);
			offset += slot.FieldShape->Size;
		}
		else if (concreteFieldType->Inlining == TypeInlining::ByReference)
		{
			slot.FieldShape = GetOrCreateShape(concreteFieldType);
			offset += sizeof(void*);
		}
		else
		{
			slot.FieldShape = GetOrCreateShape(concreteFieldType);
			offset += slot.FieldShape->Size;
		}

		shape->Slots[field->SlotIndex] = slot;
	}

	if (offset == 0 && baseType->MemoryBytesSize > 0)
		shape->Size = baseType->MemoryBytesSize;
	else
		shape->Size = offset;
}
