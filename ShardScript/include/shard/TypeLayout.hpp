#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/runtime/TypeShape.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace shard
{
    // Alignment rules for inline (by-value) object layout.
    //
    // Single source of truth for both layout engines: LayoutGenerator
    // (compile-time MemoryBytesSize / field offsets) and TypeShapeCache
    // (runtime TypeShape offsets/sizes). They MUST agree — runtime field access
    // goes through shapes while array element access goes through
    // MemoryBytesSize/GetInlineSize, and both views address the same bytes.
    //
    // Rules:
    //   - reference types: pointer alignment;
    //   - arrays: element alignment (the array header is 8 bytes, divisible by
    //     every alignment we produce, so elements stay aligned);
    //   - structs/classes: widest instance field;
    //   - field-less types (primitives): natural alignment capped at pointer size.

    inline std::size_t AlignUp(std::size_t offset, std::size_t alignment)
    {
        if (alignment <= 1)
            return offset;

        return (offset + alignment - 1) / alignment * alignment;
    }

    namespace detail
    {
        inline std::size_t GetTypeAlignmentImpl(TypeSymbol* type, std::vector<TypeSymbol*>& stack)
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
    }

    // Natural alignment of a type's inline payload.
    inline std::size_t GetTypeAlignment(TypeSymbol* type)
    {
        std::vector<TypeSymbol*> stack;
        return detail::GetTypeAlignmentImpl(type, stack);
    }

    // Alignment of a resolved runtime shape. Valid only for shapes built through
    // TypeShapeCache::BuildShape (which fills TypeShape::Alignment).
    inline std::size_t GetShapeAlignment(const TypeShape* shape)
    {
        return shape != nullptr ? shape->Alignment : 1;
    }
}
