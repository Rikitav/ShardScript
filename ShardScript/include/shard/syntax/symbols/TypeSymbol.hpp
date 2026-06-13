#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <string>
#include <vector>
#include <memory>

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

	class SHARD_API TypeSymbol : public SyntaxSymbol
	{
	public:
        std::vector<TypeSymbol*> Interfaces;
        std::vector<TypeParameterSymbol*> TypeParameters;

        std::vector<FieldSymbol*> Fields;
        std::vector<PropertySymbol*> Properties;
        std::vector<IndexatorSymbol*> Indexators;
        std::vector<ConstructorSymbol*> Constructors;
        std::vector<MethodSymbol*> Methods;

        TypeLayoutingState State = TypeLayoutingState::Unvisited;
        size_t MemoryBytesSize = 0;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsNullable = false;
        
        bool IsStatic = false;
        bool IsAbstract = false;
        bool IsSealed = false;
        
        TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }
        
        TypeSymbol(const TypeSymbol& other) = delete;
        virtual ~TypeSymbol() = default;

        size_t GetInlineSize() const;
        bool IsPrimitive();

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        virtual ConstructorSymbol* FindConstructor(const std::vector<TypeSymbol*>& parameterTypes);
        virtual MethodSymbol* FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes);
        virtual IndexatorSymbol* FindIndexator(const std::vector<TypeSymbol*>& parameterTypes);
        virtual FieldSymbol* FindField(std::wstring& name);
        virtual PropertySymbol* FindProperty(std::wstring& name);

        static bool Equals(const TypeSymbol* left, const TypeSymbol* right);

        static TypeSymbol* SubstituteType(TypeSymbol* type);
		static TypeSymbol* ReturnOf(FieldSymbol* field);
		static TypeSymbol* ReturnOf(MethodSymbol* method);
		static TypeSymbol* ReturnOf(PropertySymbol* property);
		static TypeSymbol* ReturnOf(IndexatorSymbol* indexator);
		static TypeSymbol* ReturnOf(ConstructorSymbol* constructor);
	};
}
