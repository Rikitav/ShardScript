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
		const SyntaxKind Kind;
		const std::wstring Name;
		std::wstring FullName;
		SyntaxSymbol* Parent = nullptr;

		SymbolAccesibility Accesibility = SymbolAccesibility::Private;
		bool IsExtern = false;

		inline SyntaxSymbol(const std::wstring& name, const SyntaxKind kind)
			: TypeCode(counter++), Name(name), Kind(kind) { }

		inline SyntaxSymbol(const SyntaxSymbol& other) = delete;

		inline virtual ~SyntaxSymbol() = default;

		virtual void OnSymbolDeclared(SyntaxSymbol* symbol);

		bool IsType();
		bool IsMember();
	};
}