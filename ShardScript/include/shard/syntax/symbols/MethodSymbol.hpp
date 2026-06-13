#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/symbols/MemberSymbol.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>

#include <span>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>

// Forward declarations
namespace shard
{
    class ObjectInstance;
    struct CallState;
}

namespace shard
{
    enum class MethodHandleType
    {
        None,
        Body,
        External,
        Lambda,
    };

    typedef shard::ObjectInstance* (*MethodSymbolDelegate)(const CallState& context);

    class SHARD_API MethodSymbol : public MemberSymbol
    {
        uint16_t EvalStackVariablesCount = 0;

    public:
        TypeSymbol* ReturnType = nullptr;
        std::vector<ParameterSymbol*> Parameters;

        MethodHandleType HandleType = MethodHandleType::Body;
        std::vector<std::byte> ExecutableByteCode;
        MethodSymbolDelegate FunctionPointer = nullptr;

        std::wstring LinkLibrary;
        std::wstring LinkSymbol;

        inline uint16_t GetEvalStackArgumentsCount() const
        {
            uint16_t count = static_cast<uint16_t>(Parameters.size());
            if (!IsStatic)
                count += 1; // implicit 'this'

            return count;
        }

        inline uint16_t AddVariableCount()
        {
            return EvalStackVariablesCount++;
        }

        inline uint16_t GetEvalStackVariablesCount() const
        {
            return EvalStackVariablesCount;
        }

        inline uint16_t GetEvalStackLocalsCount() const
        {
            return GetEvalStackArgumentsCount() + EvalStackVariablesCount;
        }

        inline MethodSymbol(const std::wstring& name)
            : MemberSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, MethodSymbolDelegate delegate)
            : MemberSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::External) { }

        inline MethodSymbol(const MethodSymbol& other) = delete;

        inline virtual ~MethodSymbol() override
        {
            if (FunctionPointer != nullptr)
                FunctionPointer = nullptr;
        }
    };
}
