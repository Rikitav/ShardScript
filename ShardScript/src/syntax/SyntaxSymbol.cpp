#include <shard/syntax/SyntaxSymbol.h>

using namespace shard::syntax;

bool SyntaxSymbol::IsType()
{
	return Kind == SyntaxKind::ClassDeclaration
		|| Kind == SyntaxKind::StructDeclaration
		|| Kind == SyntaxKind::CollectionExpression;
}