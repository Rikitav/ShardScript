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
    struct CallState;
    class ObjectInstance;
    class GarbageCollector;
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
        std::uint16_t EvalStackVariablesCount = 0;

    public:
        TypeSymbol* ReturnType = nullptr;
        std::vector<TypeParameterSymbol*> TypeParameters;
        std::vector<ParameterSymbol*> Parameters;

        MethodHandleType HandleType = MethodHandleType::Body;
        std::vector<std::byte> ExecutableByteCode;
        MethodSymbolDelegate FunctionPointer = nullptr;

    protected:
        inline MethodSymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind), HandleType(MethodHandleType::None) { }

    public:
        inline MethodSymbol(const std::wstring& name)
            : MemberSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, MethodSymbolDelegate delegate)
            : MemberSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::External) { }

        inline MethodSymbol(const MethodSymbol& other) = delete;

        inline virtual ~MethodSymbol() = default;

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        std::uint16_t GetEvalStackArgumentsCount() const;
        std::uint16_t GetEvalStackVariablesCount() const;
        std::uint16_t GetEvalStackLocalsCount() const;
        std::uint16_t AddVariableCount();
    };
}
