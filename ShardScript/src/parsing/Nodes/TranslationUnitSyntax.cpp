#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

using namespace shard;

TranslationUnitSyntax::TranslationUnitSyntax()
	: SyntaxNode(SyntaxKind::CompilationUnit, nullptr) { }

bool TranslationUnitSyntax::has_namespace() const
{
	return m_namespace.has_value();
}

const NamespaceDirectiveSyntax& TranslationUnitSyntax::get_namespace() const
{
	return *m_namespace;
}

std::span<const UsingDirectiveSyntax> TranslationUnitSyntax::get_usings() const
{
	return std::span<const UsingDirectiveSyntax>(m_usings.data(), m_usings.data() + m_usings.size());
}

std::span<const std::unique_ptr<MemberDeclarationSyntax>> TranslationUnitSyntax::get_members() const
{
	return std::span(m_members.data(), m_members.data() + m_members.size());
}

void TranslationUnitSyntax::add_using(UsingDirectiveSyntax& directive)
{
	m_usings.push_back(directive);
}

void shard::TranslationUnitSyntax::set_namespace(NamespaceDirectiveSyntax& directive)
{
	m_namespace = directive;
}

void shard::TranslationUnitSyntax::add_member(std::unique_ptr<MemberDeclarationSyntax> member)
{
	m_members.push_back(member);
}

TextLocation TranslationUnitSyntax::get_location() const
{
	return TextLocation(L"", 0, 0, 0);
}

void TranslationUnitSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_translation_unit(this);
}
