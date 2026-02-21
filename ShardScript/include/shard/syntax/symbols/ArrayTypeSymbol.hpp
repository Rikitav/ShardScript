#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ArrayTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		size_t Size = 0;
		int Rank = 0;

		inline ArrayTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(L"Array", SyntaxKind::ArrayType), UnderlayingType(underlayingType)
		{
			MemoryBytesSize = shard::SymbolTable::Primitives::Array->MemoryBytesSize;
			IsReferenceType = true;
		}

		inline ArrayTypeSymbol(const ArrayTypeSymbol& other) = delete;

		inline virtual ~ArrayTypeSymbol()
		{

		}

		ConstructorSymbol* FindConstructor(std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}
