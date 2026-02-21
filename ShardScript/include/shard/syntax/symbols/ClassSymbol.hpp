#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ClassSymbol : public TypeSymbol
	{
	public:
		inline ClassSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::ClassDeclaration)
		{
			IsReferenceType = true;
		}

		inline ClassSymbol(const ClassSymbol& other) = delete;

		inline virtual ~ClassSymbol() override
		{

		}
	};
}
