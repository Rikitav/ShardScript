#include <shard/syntax/SyntaxSymbol.hpp>

using namespace shard;

void SyntaxSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	// ...
}

bool SyntaxSymbol::IsType() const
{
	return Kind == SyntaxKind::ClassDeclaration
		|| Kind == SyntaxKind::StructDeclaration
		|| Kind == SyntaxKind::ArrayType
		|| Kind == SyntaxKind::DelegateType
		|| Kind == SyntaxKind::GenericType;
}

bool SyntaxSymbol::IsMember() const
{
	return Kind == SyntaxKind::MethodDeclaration
		|| Kind == SyntaxKind::PropertyDeclaration
		|| Kind == SyntaxKind::IndexatorDeclaration
		|| Kind == SyntaxKind::FieldDeclaration;
}