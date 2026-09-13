#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

void SyntaxVisitor::visit_syntax_tree(const SyntaxTree& tree)
{
	const gmt::Arena& arena = tree.get_arena();

	for (const auto& unitReference : tree.get_units())
	{
		const TranslationUnitSyntax* unit = unitReference.get();
		visit_translation_unit(unit);

		for (const gmt::Ref<UsingDirectiveSyntax>& directive : unit->get_usings().get())
			visit_using_directive(directive.get());

		if (unit->has_namespace())
			visit_namespace_directive(unit->get_namespace().get());
	}
}

void SyntaxVisitor::visit_translation_unit(const TranslationUnitSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_using_directive(const UsingDirectiveSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_namespace_directive(const NamespaceDirectiveSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_attribute(const AttributeSyntax* node)
{
	if (node == nullptr)
		return;
}
