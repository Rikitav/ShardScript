#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ClassSymbol : public TypeSymbol
	{
	public:
		inline ClassSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::ClassDeclaration)
		{
			Inlining = TypeInlining::ByReference;
		}

		inline ClassSymbol(const ClassSymbol& other) = delete;

		inline virtual ~ClassSymbol() override
		{

		}
	};
}
