#include <shard/parsing/nodes/members/ClassDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ClassDeclarationSyntax::ClassDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

TextLocation ClassDeclarationSyntax::get_location() const
{
	TextLocation location = get_declare_token().get_location();

	if (!get_close_bracket().is_missing())
		return TextLocation(location, get_close_bracket().get_location());

	if (!get_semicolon().is_missing())
		return TextLocation(location, get_semicolon().get_location());

	if (!get_identifier().is_missing())
		return TextLocation(location, get_identifier().get_location());

	return location;
}

void ClassDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_class_declaration(this);
}
