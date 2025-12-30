#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/TypeInfo.h>
#include <shard/parsing/semantic/SymbolInfo.h>
#include <shard/parsing/semantic/NamespaceTree.h>

namespace shard
{
	class SHARD_API SemanticModel
	{
	public:
		shard::SyntaxTree& Tree;
		shard::SymbolTable* Table;
		shard::NamespaceTree* Namespaces;

		SemanticModel(shard::SyntaxTree& tree);
		~SemanticModel();

		shard::SymbolInfo GetSymbolInfo(shard::SyntaxNode* node);
		shard::TypeInfo GetTypeInfo(shard::ExpressionSyntax* expression);
		//Conversion ClassifyConversion(shard::ExpressionSyntax* expression, shard::TypeSymbol destination);
		//DataFlowAnalysis AnalyzeDataFlow(shard::SyntaxNode* firstNode, shard::SyntaxNode* lastNode);
	};
}