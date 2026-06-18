#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/symbols/MemberSymbol.hpp>

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
    class FieldSymbol;
    class PropertySymbol;
    class IndexatorSymbol;
}

namespace shard
{
    enum class TypeLayoutingState
    {
        Unvisited,
        Visiting,
        Visited
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
        std::vector<MethodSymbol*> Methods;

        std::unordered_map<MethodSymbol*, MethodSymbol*> InterfaceMethodMap;

        TypeLayoutingState State = TypeLayoutingState::Unvisited;
        std::size_t MemoryBytesSize = 0;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsNullable = false;
        
        /*
        bool IsAbstract = false;
        bool IsSealed = false;
        */
        
        TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind) { }
        
        TypeSymbol(const TypeSymbol& other) = delete;
        virtual ~TypeSymbol() = default;

        std::size_t GetInlineSize() const;
        bool IsPrimitive();

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        virtual ConstructorSymbol* FindConstructor(const std::vector<TypeSymbol*>& parameterTypes);
        virtual MethodSymbol* FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes);
        virtual IndexatorSymbol* FindIndexator(const std::vector<TypeSymbol*>& parameterTypes);
        virtual FieldSymbol* FindField(std::wstring& name);
        virtual PropertySymbol* FindProperty(std::wstring& name);

        MethodSymbol* FindInterfaceImplementation(MethodSymbol* interfaceMethod);

        static bool Equals(const TypeSymbol* left, const TypeSymbol* right);
        static bool IsAssignableFrom(const TypeSymbol* target, const TypeSymbol* source);

        static TypeSymbol* SubstituteType(TypeSymbol* type);
		static TypeSymbol* ReturnOf(FieldSymbol* field);
		static TypeSymbol* ReturnOf(MethodSymbol* method);
		static TypeSymbol* ReturnOf(PropertySymbol* property);
		static TypeSymbol* ReturnOf(IndexatorSymbol* indexator);
		static TypeSymbol* ReturnOf(ConstructorSymbol* constructor);
	};
}
