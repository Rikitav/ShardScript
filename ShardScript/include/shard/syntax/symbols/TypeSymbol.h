#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <string>
#include <vector>

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
        TypeSymbol* BaseType = nullptr;
        std::vector<TypeSymbol*> Interfaces;
        std::vector<TypeParameterSymbol*> TypeParameters;

        std::vector<ConstructorSymbol*> Constructors;
        std::vector<MethodSymbol*> Methods;
        std::vector<IndexatorSymbol*> Indexators;
        std::vector<FieldSymbol*> Fields;
        std::vector<PropertySymbol*> Properties;

        TypeLayoutingState State = TypeLayoutingState::Unvisited;
        size_t MemoryBytesSize = 0;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsNullable = false;
        
        bool IsStatic = false;
        bool IsAbstract = false;
        bool IsSealed = false;
        
        inline TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline TypeSymbol(const TypeSymbol& other) = delete;

        inline ~TypeSymbol() override
        {
#pragma warning (push)
#pragma warning (disable: 4150)
            for (MethodSymbol* methodSymbol : Methods)
                delete methodSymbol;

            for (FieldSymbol* fieldSymbol : Fields)
                delete fieldSymbol;

            for (PropertySymbol* propertySymbol : Properties)
                delete propertySymbol;

            for (IndexatorSymbol* indexatorSymbol : Indexators)
                delete indexatorSymbol;

            for (ConstructorSymbol* ctorSymbol : Constructors)
                delete ctorSymbol;

            for (TypeParameterSymbol* typeParamSymbol : TypeParameters)
                delete typeParamSymbol;
#pragma warning (pop)

            if (BaseType != nullptr)
                BaseType = nullptr;
        }

        inline size_t GetInlineSize() const
        {
            return IsReferenceType ? sizeof(void*) : MemoryBytesSize;
        }

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        virtual ConstructorSymbol* FindConstructor(std::vector<TypeSymbol*> parameterTypes);
        virtual MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes);
        virtual IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes);
        virtual FieldSymbol* FindField(std::wstring& name);
        virtual PropertySymbol* FindProperty(std::wstring& name);

        static bool Equals(const TypeSymbol* left, const TypeSymbol* right);
        bool IsPrimitive();

        static TypeSymbol* SubstituteType(TypeSymbol* type);
		static TypeSymbol* ReturnOf(FieldSymbol* field);
		static TypeSymbol* ReturnOf(MethodSymbol* method);
		static TypeSymbol* ReturnOf(PropertySymbol* property);
		static TypeSymbol* ReturnOf(IndexatorSymbol* indexator);
		static TypeSymbol* ReturnOf(ConstructorSymbol* constructor);
	};
}
