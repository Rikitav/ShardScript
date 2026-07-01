#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declaration
namespace shard
{
    class TypeParameterSymbol;
    class MethodSymbol;
    class ConstructorSymbol;
    class OperatorSymbol;
    class FieldSymbol;
    class PropertySymbol;
    class InterfaceSymbol;
    class IndexatorSymbol;
    class CallStackFrame;
    class GenericTypeSymbol;
}

namespace shard
{
    enum class TypeLayoutingState
    {
        Unvisited,
        Visiting,
        Visited
    };

    enum class TypeInlining
    {
        ByReference,
        ByValue
    };

	class SHARD_API TypeSymbol : public MemberSymbol
	{
	public:
        std::vector<TypeSymbol*> Interfaces;
        std::vector<TypeParameterSymbol*> TypeParameters;

        std::vector<FieldSymbol*> Fields;
        std::vector<PropertySymbol*> Properties;
        std::vector<IndexatorSymbol*> Indexators;
        std::vector<ConstructorSymbol*> Constructors;
        std::vector<OperatorSymbol*> Operators;
        std::vector<MethodSymbol*> Methods;

        std::unordered_map<MethodSymbol*, MethodSymbol*> InterfaceMethodMap;

        TypeLayoutingState LayoutingState = TypeLayoutingState::Unvisited;
        TypeInlining Inlining = TypeInlining::ByReference;

        std::size_t MemoryBytesSize = 0;
        bool IsNullable = false;

        inline bool IsReferenceType() const { return Inlining == TypeInlining::ByReference; }
        
        TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind) { }
        
        TypeSymbol(const TypeSymbol& other) = delete;
        virtual ~TypeSymbol() = default;

        std::size_t GetInlineSize() const;
        bool IsPrimitive();

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;
        bool IsType() const override;
        bool IsMember() const override;

        virtual ConstructorSymbol* FindConstructor(const std::vector<TypeSymbol*>& parameterTypes);
        virtual MethodSymbol* FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes);
        virtual OperatorSymbol* FindOperator(TokenType opToken, const std::vector<TypeSymbol*>& parameterTypes);
        virtual IndexatorSymbol* FindIndexator(const std::vector<TypeSymbol*>& parameterTypes);
        virtual FieldSymbol* FindField(std::wstring& name);
        virtual PropertySymbol* FindProperty(std::wstring& name);

        virtual MethodSymbol* FindInterfaceImplementation(MethodSymbol* interfaceMethod);

        static bool Equals(const TypeSymbol* expected, const TypeSymbol* actual);
        static bool IsAssignableFrom(const TypeSymbol* target, const TypeSymbol* source);
        
        static TypeSymbol* SubstituteType(TypeSymbol* type);
		static TypeSymbol* ReturnOf(FieldSymbol* field);
		static TypeSymbol* ReturnOf(MethodSymbol* method);
		static TypeSymbol* ReturnOf(PropertySymbol* property);
		static TypeSymbol* ReturnOf(ConstructorSymbol* constructor);
	};
}
