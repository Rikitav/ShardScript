#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class ArrayTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		size_t Size = 0;
		int Rank = 0;

		inline ArrayTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(L"Array", SyntaxKind::ArrayType), UnderlayingType(underlayingType)
		{
			IsReferenceType = true;
		}

		inline ~ArrayTypeSymbol() = default;

		MethodSymbol* FindConstructor(std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}
