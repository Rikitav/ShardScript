#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/SymbolInfo.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/TypeInfo.hpp>

#include <stdexcept>
#include <memory>

using namespace shard;

SemanticModel::SemanticModel(shard::SyntaxTree& tree) : Tree(tree)
{
	Table = std::make_unique<SymbolTable>();
	Namespaces = std::make_unique<NamespaceTree>();
	//Namespaces->Root = SymbolTable::Global::Namespace->Node;
}

SemanticModel::~SemanticModel()
{
}

SymbolInfo SemanticModel::GetSymbolInfo(SyntaxNode* node)
{
	auto symbol = Table->LookupSymbol(node);
	return SymbolInfo(symbol.value_or(nullptr));
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

				case TokenType::DoubleLiteral:
					return SymbolTable::Primitives::Double;

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