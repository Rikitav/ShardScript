#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolInfo.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/TypeInfo.h>

#include <stdexcept>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::parsing::semantic;

SemanticModel::SemanticModel(shard::parsing::lexical::SyntaxTree& tree) : Tree(tree)
{
	Table = new SymbolTable();
}

SemanticModel::~SemanticModel()
{
	delete Table;
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
			throw runtime_error("unsupported expression type");

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
					throw runtime_error("unsupported literal type");
			}
		}
	}
}