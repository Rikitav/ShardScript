#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/TypeInfo.hpp>

#include <memory>
#include <shard/parsing/semantic/SymbolInfo.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

namespace shard
{
	class SHARD_API SemanticModel
	{
	public:
		shard::SyntaxTree& Tree;
		std::unique_ptr<shard::SymbolTable> Table;
		std::unique_ptr<shard::NamespaceTree> Namespaces;

		SemanticModel(shard::SyntaxTree& tree);
		~SemanticModel();

		SemanticModel(const SemanticModel&) = delete;
		SemanticModel& operator=(const SemanticModel&) = delete;

		shard::SymbolInfo GetSymbolInfo(shard::SyntaxNode* node);
		shard::TypeInfo GetTypeInfo(shard::ExpressionSyntax* expression);
		//Conversion ClassifyConversion(shard::ExpressionSyntax* expression, shard::TypeSymbol destination);
		//DataFlowAnalysis AnalyzeDataFlow(shard::SyntaxNode* firstNode, shard::SyntaxNode* lastNode);
	};
}