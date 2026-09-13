#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

using namespace shard;

TranslationUnitSyntax::TranslationUnitSyntax()
	: SyntaxNode(SyntaxKind::CompilationUnit, gmt::Ref<SyntaxNode>()) { }

bool TranslationUnitSyntax::has_namespace() const
{
	return !m_namespace.is_null();
}

gmt::Ref<const NamespaceDirectiveSyntax> TranslationUnitSyntax::get_namespace() const
{
	return m_namespace;
}

gmt::Span<const gmt::Ref<UsingDirectiveSyntax>> TranslationUnitSyntax::get_usings() const
{
	return m_usings;
}

gmt::Span<const gmt::Ref<MemberDeclarationSyntax>> TranslationUnitSyntax::get_members() const
{
	return m_members;
}

void shard::TranslationUnitSyntax::set_namespace(gmt::Ref<NamespaceDirectiveSyntax> directive)
{
	m_namespace = directive;
}

void shard::TranslationUnitSyntax::set_usings(gmt::Span<gmt::Ref<UsingDirectiveSyntax>> usings)
{
	m_usings = usings;
}

void shard::TranslationUnitSyntax::set_members(gmt::Span<gmt::Ref<MemberDeclarationSyntax>> members)
{
	m_members = members;
}

TextLocation TranslationUnitSyntax::get_location() const
{
	return TextLocation(L"", 0, 0, 0);
}

void TranslationUnitSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_translation_unit(this);
}
