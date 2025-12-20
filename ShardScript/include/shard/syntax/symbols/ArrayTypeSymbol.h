#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class SHARD_API ArrayTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		size_t Size = 0;
		int Rank = 0;

		inline ArrayTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(L"Array", SyntaxKind::ArrayType), UnderlayingType(underlayingType)
		{
			MemoryBytesSize = shard::parsing::semantic::SymbolTable::Primitives::Array->MemoryBytesSize;
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
