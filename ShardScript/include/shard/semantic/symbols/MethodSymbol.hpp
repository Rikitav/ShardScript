#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/MethodEffectSummary.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

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
    class ClassSymbol;
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

    using MethodSymbolDelegate         = shard::ObjectInstance* (*)(const CallState& context);
    using NativeMethodCallback         = shard::ObjectInstance* (*)(const CallState& context);
    using NativeInstanceMethodCallback = shard::ObjectInstance* (*)(ObjectInstance* self, const CallState& context);
    using NativeConstructorCallback    = shard::ObjectInstance* (*)(const CallState& context);

    struct SHARD_API FrameSlotRecipe
    {
        TypeSymbol* ConcreteType = nullptr; // nullptr => TypeParameterIndex, or unknown
        std::int16_t TypeParameterIndex = -1;
    };

    struct SHARD_API FrameLayout
    {
        static constexpr std::size_t EntryHeaderSize = sizeof(void*);
        std::vector<FrameSlotRecipe> VariableSlots;

        std::uint32_t MaxEvalDepth = 0;
        std::size_t EvalSlotPayload = 0;

        static std::size_t ResolveTypePayload(TypeSymbol* type);
        std::size_t ComputeLocalsBytes(const MethodSymbol& method) const;
    };

    class SHARD_API MethodSymbol : public MemberSymbol
    {
        std::uint16_t EvalStackVariablesCount = 0;

    public:
        TypeSymbol* ReturnType = nullptr;
        std::vector<TypeParameterSymbol*> TypeParameters;
        std::vector<ParameterSymbol*> Parameters;

        MethodHandleType HandleType = MethodHandleType::None;
        std::vector<std::byte> ExecutableByteCode;
        MethodSymbolDelegate FunctionPointer = nullptr;

        bool IsAbstract = false;
        bool IsAsync = false;

        // Populated by async state-machine lowering.
        ClassSymbol* AsyncStateMachineClass = nullptr;
        MethodSymbol* AsyncStateMachineMoveNext = nullptr;

        // Side-effect / mutation summary computed by FlowAnalyzer.
        MethodEffectSummary EffectSummary;
        bool EffectsComputed = false;

        // Frame memory requirements for the stack-allocated frame.
        FrameLayout Layout;

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

        bool IsMethod() const override;
        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        std::uint16_t GetEvalStackArgumentsCount() const;
        std::uint16_t GetEvalStackVariablesCount() const;
        std::uint16_t GetEvalStackLocalsCount() const;
        std::uint16_t AddVariableCount(TypeSymbol* type = nullptr);

        // Fills a variable slot recipe that was allocated before its type was
        // known (e.g. DeclarationCollector). No-op if the slot already has a
        // type or the index is out of range.
        void SealVariableSlot(std::uint16_t slotIndex, TypeSymbol* type);
    };
}
