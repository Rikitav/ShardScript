#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace shard
{
	class SHARD_API GenericTypeSymbol : public TypeSymbol
	{
		std::unordered_map<TypeSymbol*, TypeSymbol*> _typeParametersMap;

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

		void AddTypeParameter(TypeSymbol* typeParam, TypeSymbol* constraintType);
		TypeSymbol* SubstituteTypeParameters(TypeSymbol* typeParam);

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}