#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <vector>

SHARD_API void SetAccesibility(shard::syntax::SyntaxSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
SHARD_API void SetAccesibility(shard::syntax::symbols::TypeSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
SHARD_API void SetAccesibility(shard::syntax::symbols::FieldSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
SHARD_API void SetAccesibility(shard::syntax::symbols::PropertySymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
SHARD_API void SetAccesibility(shard::syntax::symbols::MethodSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
