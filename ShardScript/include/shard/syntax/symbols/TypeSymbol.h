#pragma once
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
    class MethodSymbol;
    class FieldSymbol;

    enum class TypeLayoutingState
    {
        Unvisited,
        Visiting,
        Visited
    };

	class TypeSymbol : public SyntaxSymbol
	{
	public:
        TypeSymbol* BaseType = nullptr;
        std::vector<TypeSymbol*> Interfaces;

        std::vector<MethodSymbol*> Methods;
        std::vector<FieldSymbol*> Fields;

        TypeLayoutingState State = TypeLayoutingState::Unvisited;
        size_t MemoryBytesSize = 0;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsStatic = false;
        bool IsAbstract = false;
        bool IsSealed = false;

        inline TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline ~TypeSymbol() override
        {
            for (MethodSymbol* methodSymbol : Methods)
                delete methodSymbol;

            for (FieldSymbol* fieldSymbol : Fields)
                delete fieldSymbol;

            if (BaseType != nullptr)
                BaseType = nullptr;
        }

        MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes);
        FieldSymbol* FindField(std::wstring& name);
	};
}
