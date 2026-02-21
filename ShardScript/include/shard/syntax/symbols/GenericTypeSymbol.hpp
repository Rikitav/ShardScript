#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

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