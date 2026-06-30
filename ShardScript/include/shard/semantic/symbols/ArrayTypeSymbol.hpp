#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ArrayTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		std::size_t Length = 0;

		inline ArrayTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(L"Array", SyntaxKind::ArrayType), UnderlayingType(underlayingType)
		{
			MemoryBytesSize = shard::SymbolTable::Primitives::Array->MemoryBytesSize;
			Inlining = TypeInlining::ByReference;
		}

		inline ArrayTypeSymbol(const ArrayTypeSymbol& other) = delete;

		inline virtual ~ArrayTypeSymbol()
		{

		}

		ConstructorSymbol* FindConstructor(const std::vector<TypeSymbol*>& parameterTypes) override;
		MethodSymbol* FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes) override;
		MethodSymbol* FindInterfaceImplementation(MethodSymbol* interfaceMethod) override;
		IndexatorSymbol* FindIndexator(const std::vector<TypeSymbol*>& parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}
