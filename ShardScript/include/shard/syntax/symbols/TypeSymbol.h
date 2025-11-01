#pragma once
//#include <shard/syntax/symbols/FieldSymbol.h>
//#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <unordered_map>
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
        size_t MemoryBytesSize = -1;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsStatic = false;
        bool IsAbstract = false;
        bool IsSealed = false;

        inline TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes);
        FieldSymbol* FindField(std::wstring& name);
	};
}
