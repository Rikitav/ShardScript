#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/TokenType.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>
#include <shard/parsing/semantic/SymbolInfo.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/TypeInfo.hpp>

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