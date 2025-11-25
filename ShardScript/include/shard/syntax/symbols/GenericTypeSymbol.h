#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace shard::syntax::symbols
{
	class GenericTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		std::unordered_map<std::wstring, TypeSymbol*> GenericTypes;

		inline GenericTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(underlayingType->Name, underlayingType->Kind), UnderlayingType(underlayingType)
		{
			IsReferenceType = underlayingType->IsReferenceType;
		}

		inline ~GenericTypeSymbol() = default;

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}