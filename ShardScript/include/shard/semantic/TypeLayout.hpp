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
    namespace detail
    {
        std::size_t GetTypeAlignmentImpl(TypeSymbol* type, std::vector<TypeSymbol*>& stack);
    }

    std::size_t AlignUp(std::size_t offset, std::size_t alignment);
    std::size_t GetTypeAlignment(TypeSymbol* type);
    std::size_t GetShapeAlignment(const TypeShape* shape);
}
