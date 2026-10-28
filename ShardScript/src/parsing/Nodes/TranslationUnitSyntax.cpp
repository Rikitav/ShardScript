#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

using namespace shard;

TranslationUnitSyntax::TranslationUnitSyntax()
	: SyntaxNode(SyntaxKind::CompilationUnit, nullptr) { }

bool TranslationUnitSyntax::has_namespace() const
{
	return !(m_namespace == nullptr);
}

const NamespaceDirectiveSyntax* TranslationUnitSyntax::get_namespace() const
{
	return m_namespace;
}

std::span<UsingDirectiveSyntax* const> TranslationUnitSyntax::get_usings() const
{
	return m_usings;
}

std::span<MemberDeclarationSyntax* const> TranslationUnitSyntax::get_members() const
{
	return m_members;
}

void shard::TranslationUnitSyntax::set_namespace(NamespaceDirectiveSyntax* directive)
{
	m_namespace = directive;
}

void shard::TranslationUnitSyntax::set_usings(std::span<UsingDirectiveSyntax*> usings)
{
	m_usings = usings;
}

void shard::TranslationUnitSyntax::set_members(std::span<MemberDeclarationSyntax*> members)
{
	m_members = members;
}

TextLocation TranslationUnitSyntax::get_location() const
{
	TextLocation location;

	if (!m_usings.empty())
		location = TextLocation(location, m_usings.front()->get_location());

	if (has_namespace())
		location = TextLocation(location, m_namespace->get_location());

	if (!m_members.empty())
	{
		std::span<MemberDeclarationSyntax* const> members = m_members;
		location = TextLocation(location, members.front()->get_location());
		location = TextLocation(location, members.back()->get_location());
	}

	return location;
}

void TranslationUnitSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_translation_unit(this);
}