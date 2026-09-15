#include <shard/parsing/nodes/members/ClassDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ClassDeclarationSyntax::ClassDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

TextLocation ClassDeclarationSyntax::get_location() const
{
	if (!get_identifier().is_missing())
		return get_identifier().get_location();

	return get_declare_token().get_location();
}

void ClassDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_class_declaration(this);
}
