#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <string>

namespace shard::syntax
{
	class SHARD_API SyntaxSymbol
	{
		inline static int counter = 0;

	public:
		const int TypeCode;
		const std::wstring Name;
		const SyntaxKind Kind;
		std::wstring FullName;
		SyntaxSymbol* Parent = nullptr;
		SymbolAccesibility Accesibility = SymbolAccesibility::Private;

		inline SyntaxSymbol(const std::wstring& name, const SyntaxKind kind)
			: TypeCode(counter++), Name(name), Kind(kind) { }

		inline SyntaxSymbol(const SyntaxSymbol& other) = delete;

		inline virtual ~SyntaxSymbol()
		{

		}

		bool IsType();
		bool IsMember();
	};
}