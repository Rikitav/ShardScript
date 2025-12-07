#include <shard/syntax/SyntaxSymbol.h>

using namespace shard::syntax;

void SyntaxSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	// ...
}

bool SyntaxSymbol::IsType()
{
	return Kind == SyntaxKind::ClassDeclaration
		|| Kind == SyntaxKind::StructDeclaration
		|| Kind == SyntaxKind::ArrayType
		|| Kind == SyntaxKind::DelegateType
		|| Kind == SyntaxKind::GenericType;
}

bool SyntaxSymbol::IsMember()
{
	return Kind == SyntaxKind::MethodDeclaration
		|| Kind == SyntaxKind::PropertyDeclaration
		|| Kind == SyntaxKind::FieldDeclaration;
}