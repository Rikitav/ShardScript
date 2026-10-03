#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

StructDeclarationSyntax::StructDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent) { }

TextLocation StructDeclarationSyntax::get_location() const
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

void StructDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_struct_declaration(this);
}
