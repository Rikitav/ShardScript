#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace shard
{
	class SHARD_API GenericTypeSymbol : public TypeSymbol
	{
		std::unordered_map<TypeParameterSymbol*, TypeSymbol*> _typeParametersMap;

	public:
		TypeSymbol* UnderlayingType = nullptr;

		inline GenericTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(underlayingType->Name, SyntaxKind::GenericType), UnderlayingType(underlayingType)
		{
			IsReferenceType = underlayingType->IsReferenceType;
		}

		inline GenericTypeSymbol(const GenericTypeSymbol& other) = delete;

		inline virtual ~GenericTypeSymbol()
		{

		}

		void AddTypeParameter(TypeParameterSymbol* typeParam, TypeSymbol* constraintType);
		TypeSymbol* SubstituteTypeParameters(TypeParameterSymbol* typeParam);

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}