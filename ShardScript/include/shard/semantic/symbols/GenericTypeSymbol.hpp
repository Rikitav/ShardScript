#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <string>
#include <vector>
#include <unordered_map>

namespace shard
{
	class SHARD_API GenericTypeSymbol : public TypeSymbol
	{
		std::unordered_map<TypeParameterSymbol*, TypeSymbol*> _typeParametersMap;

		static std::wstring BuildDisplayName(TypeSymbol* underlayingType, const std::unordered_map<std::wstring, TypeSymbol*>& typeArgs)
		{
			std::wstring displayName = underlayingType->Name + L"<";
			for (std::size_t i = 0; i < underlayingType->TypeParameters.size(); ++i)
			{
				if (i > 0)
					displayName += L", ";

				TypeParameterSymbol* typeParam = underlayingType->TypeParameters[i];
				auto it = typeArgs.find(typeParam->Name);
				TypeSymbol* arg = it != typeArgs.end() ? it->second : typeParam;
				displayName += TypeSymbol::GetDisplayName(arg);
			}
			displayName += L">";
			return displayName;
		}

	public:
		TypeSymbol* UnderlayingType = nullptr;

		inline GenericTypeSymbol(TypeSymbol* underlayingType, const std::unordered_map<std::wstring, TypeSymbol*>& typeArgs = {})
			: TypeSymbol(BuildDisplayName(underlayingType, typeArgs), SyntaxKind::GenericType), UnderlayingType(underlayingType)
		{
			FullName = Name;
			Inlining = UnderlayingType->Inlining;
			//Inlining = TypeInlining::ByReference;
		}

		inline GenericTypeSymbol(const GenericTypeSymbol& other) = delete;

		inline virtual ~GenericTypeSymbol()
		{

		}

		void AddTypeParameter(TypeParameterSymbol* typeParam, TypeSymbol* constraintType);
		TypeSymbol* SubstituteTypeParameters(TypeParameterSymbol* typeParam);

		std::unordered_map<FieldSymbol*, std::size_t> FieldOffsets;
		std::size_t GetFieldOffset(FieldSymbol* field) const;

		MethodSymbol* FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes) override;
		IndexatorSymbol* FindIndexator(const std::vector<TypeSymbol*>& parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
		MethodSymbol* FindInterfaceImplementation(MethodSymbol* interfaceMethod) override;
	};
}