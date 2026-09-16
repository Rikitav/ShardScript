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
	TextLocation location;

	if (!m_usings.empty())
		location = TextLocation(location, m_usings.as_span().front().as_ptr()->get_location());

	if (has_namespace())
		location = TextLocation(location, m_namespace.as_ptr()->get_location());

	if (!m_members.empty())
	{
		std::span<const gmt::Ref<MemberDeclarationSyntax>> members = m_members.as_span();
		location = TextLocation(location, members.front().as_ptr()->get_location());
		location = TextLocation(location, members.back().as_ptr()->get_location());
	}

	return location;
}

void TranslationUnitSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_translation_unit(this);
}
