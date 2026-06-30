#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/TypeInfo.hpp>

#include <memory>
#include <shard/semantic/SymbolInfo.hpp>
#include <shard/semantic/NamespaceTree.hpp>

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