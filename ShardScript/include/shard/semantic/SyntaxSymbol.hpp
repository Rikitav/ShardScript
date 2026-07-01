#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <string>

namespace shard
{
	enum class SymbolAccesibility
	{
		Private,
		Public
	};

	enum class SymbolLinking
	{
		Static,
		Instance
	};

	class SHARD_API SyntaxSymbol
	{
		inline static int counter = 0;

	public:
		const int DefinitionIndex;
		const SyntaxKind Kind;
		const std::wstring Name;
		std::wstring FullName;
		SyntaxSymbol* Parent = nullptr;

		SymbolAccesibility Accesibility = SymbolAccesibility::Private;

		inline SyntaxSymbol(const std::wstring& name, const SyntaxKind kind)
			: DefinitionIndex(counter++), Name(name), FullName(name), Kind(kind) { }

		inline SyntaxSymbol(const SyntaxSymbol& other) = delete;

		inline virtual ~SyntaxSymbol() = default;
		
		
		virtual void OnSymbolDeclared(SyntaxSymbol* symbol) { }
        virtual bool IsType() const { return false; }
        virtual bool IsMember() const { return false; }
        virtual bool IsMethod() const { return false; }
	};

	constexpr SymbolLinking LINK_STATIC = shard::SymbolLinking::Static;
	constexpr SymbolLinking LINK_INSTANCE = shard::SymbolLinking::Instance;

	constexpr shard::SymbolAccesibility ACS_PUBLIC = shard::SymbolAccesibility::Public;
	constexpr shard::SymbolAccesibility ACS_PRIVATE = shard::SymbolAccesibility::Private;
}