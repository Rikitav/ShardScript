#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>

#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/NamespaceTree.h>
#include <shard/parsing/semantic/SymbolInfo.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/TypeInfo.h>

#include <stdexcept>

using namespace shard;

SemanticModel::SemanticModel(shard::SyntaxTree& tree) : Tree(tree)
{
	Table = new SymbolTable();
	Namespaces = new NamespaceTree();
}

SemanticModel::~SemanticModel()
{
	delete Table;
	delete Namespaces;
}

SymbolInfo SemanticModel::GetSymbolInfo(SyntaxNode* node)
{
	return SymbolInfo(Table->LookupSymbol(node));
}

TypeInfo SemanticModel::GetTypeInfo(ExpressionSyntax* expression)
{
	switch (expression->Kind)
	{
		default:
			throw std::runtime_error("unsupported expression type");

		case SyntaxKind::LiteralExpression:
		{
			LiteralExpressionSyntax* literal = static_cast<LiteralExpressionSyntax*>(expression);
			switch (literal->LiteralToken.Type)
			{
				case TokenType::BooleanLiteral:
					return SymbolTable::Primitives::Boolean;

				case TokenType::NumberLiteral:
					return SymbolTable::Primitives::Integer;

				case TokenType::CharLiteral:
					return SymbolTable::Primitives::Char;

				case TokenType::StringLiteral:
					return SymbolTable::Primitives::String;

				default:
					throw std::runtime_error("unsupported literal type");
			}
		}
	}
}