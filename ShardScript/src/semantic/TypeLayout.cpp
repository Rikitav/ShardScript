#include <shard/semantic/TypeLayout.hpp>

using namespace shard;

std::size_t shard::detail::GetTypeAlignmentImpl(TypeSymbol* type, std::vector<TypeSymbol*>& stack)
{
    if (type == nullptr)
        return 1;

    // By-value cycles are rejected with a diagnostic elsewhere; break the
    // recursion here so alignment computation cannot loop before that fires.
    if (std::find(stack.begin(), stack.end(), type) != stack.end())
        return 1;

    if (type->Inlining == TypeInlining::ByReference)
        return sizeof(void*);

    if (type->Kind == SyntaxKind::TypeParameter)
        return sizeof(void*); // unresolved generic argument: conservative

    stack.push_back(type);

    std::size_t alignment = 1;
    if (type->Kind == SyntaxKind::ArrayType)
    {
        alignment = GetTypeAlignmentImpl(static_cast<ArrayTypeSymbol*>(type)->UnderlayingType, stack);
    }
    else if (type->Kind == SyntaxKind::GenericType)
    {
        GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
        for (FieldSymbol* field : generic->UnderlayingType->Fields)
        {
            if (field->Linking != LINK_INSTANCE)
                continue;

            TypeSymbol* fieldType = field->ReturnType;
            if (fieldType == nullptr)
                continue;

            if (fieldType->Kind == SyntaxKind::TypeParameter)
                fieldType = generic->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(fieldType));
            if (fieldType == nullptr)
                continue;

            alignment = std::max(alignment, GetTypeAlignmentImpl(fieldType, stack));
        }
    }
    else if (!type->Fields.empty())
    {
        for (FieldSymbol* field : type->Fields)
        {
            if (field->Linking != LINK_INSTANCE)
                continue;
            if (field->ReturnType == nullptr)
                continue;

            alignment = std::max(alignment, GetTypeAlignmentImpl(field->ReturnType, stack));
        }
    }
    else
    {
        // Field-less type (primitive): natural alignment capped at pointer size.
        alignment = std::min<std::size_t>(std::max<std::size_t>(type->MemoryBytesSize, 1), sizeof(void*));
    }

    stack.pop_back();
    return alignment;
}

std::size_t shard::AlignUp(std::size_t offset, std::size_t alignment)
{
    if (alignment <= 1)
        return offset;

    return (offset + alignment - 1) / alignment * alignment;
}

// Natural alignment of a type's payload.
std::size_t shard::GetTypeAlignment(TypeSymbol* type)
{
    std::vector<TypeSymbol*> stack;
    return detail::GetTypeAlignmentImpl(type, stack);
}

// Alignment of a resolved runtime shape. Valid only for shapes built through
// TypeShapeCache::BuildShape (which fills TypeShape::Alignment).
std::size_t shard::GetShapeAlignment(const TypeShape* shape)
{
    return shape != nullptr ? shape->Alignment : 1;
}