#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/TypeInfo.h>
#include <shard/parsing/semantic/SymbolInfo.h>

namespace shard::parsing::semantic
{
	class SemanticModel
	{
	public:
		shard::parsing::lexical::SyntaxTree& Tree;
		shard::parsing::semantic::SymbolTable* Table;

		SemanticModel(shard::parsing::lexical::SyntaxTree& tree);
		~SemanticModel();

		shard::parsing::semantic::SymbolInfo GetSymbolInfo(shard::syntax::SyntaxNode* node);
		shard::parsing::semantic::TypeInfo GetTypeInfo(shard::syntax::nodes::ExpressionSyntax* expression);
		//Conversion ClassifyConversion(shard::syntax::nodes::ExpressionSyntax* expression, shard::syntax::symbols::TypeSymbol destination);
		//DataFlowAnalysis AnalyzeDataFlow(shard::syntax::SyntaxNode* firstNode, shard::syntax::SyntaxNode* lastNode);
	};
}