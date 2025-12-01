#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace shard::syntax::symbols
{
	class SHARD_API GenericTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		std::unordered_map<std::wstring, TypeSymbol*> GenericTypes;

		inline GenericTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(underlayingType->Name, underlayingType->Kind), UnderlayingType(underlayingType)
		{
			IsReferenceType = underlayingType->IsReferenceType;
		}

		inline GenericTypeSymbol(const GenericTypeSymbol& other) = delete;

		inline virtual ~GenericTypeSymbol()
		{

		}

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}