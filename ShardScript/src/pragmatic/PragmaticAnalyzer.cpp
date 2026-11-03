#include <shard/pragmatic/PragmaticAnalyzer.hpp>

#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/members/InterfaceDeclarationSyntax.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <shard/pragmatic/EscapeAnalyzer.hpp>
#include <shard/pragmatic/LayoutGenerator.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

#include <optional>

using namespace shard;

namespace
{
	void assign_type_slots(StructSymbol* type, PragmaticModel& pragmatic)
	{
		if (type == nullptr)
			return;

		for (MethodSymbol* member : type->methods)
			pragmatic.reserve_interface_slot(member);

		for (PropertySymbol* property : type->properties)
		{
			if (property == nullptr)
				continue;

			pragmatic.reserve_interface_slot(property->get_getter());
			pragmatic.reserve_interface_slot(property->get_setter());
		}

		for (IndexatorSymbol* indexator : type->indexators)
		{
			if (indexator == nullptr)
				continue;

			pragmatic.reserve_interface_slot(indexator->get_getter());
			pragmatic.reserve_interface_slot(indexator->get_setter());
		}
	}

	void assign_namespace_slots(NamespaceSymbol* space, PragmaticModel& pragmatic)
	{
		for (SyntaxSymbol* member : space->members)
		{
			if (member != nullptr && member->get_kind() == SyntaxKind::InterfaceDeclaration)
				assign_type_slots(static_cast<StructSymbol*>(member), pragmatic);
		}

		for (const auto& child : space->get_children())
			assign_namespace_slots(child.second, pragmatic);
	}

	// every virtual member of every interface gets a stable slot; the emitter
	// encodes it and each type's shape carries the resolved implementation
	class interface_slot_visitor final : public SemanticVisitor
	{
		PragmaticModel& m_pragmatic;

	public:
		interface_slot_visitor(SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics)
			: SemanticVisitor(model, diagnostics)
			, m_pragmatic(pragmatic)
		{ }

	protected:
		void visit_interface_declaration(const InterfaceDeclarationSyntax* node) override
		{
			std::optional<const InterfaceDeclarationSyntax*> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				assign_type_slots(lookup_symbol<InterfaceSymbol>(*nodeRef), m_pragmatic);

			SyntaxVisitor::visit_interface_declaration(node);
		}
	};
}

PragmaticAnalyzer::PragmaticAnalyzer(DiagnosticsContext& diagnostics)
	: m_diagnostics(diagnostics)
{ }

void PragmaticAnalyzer::analyze(SyntaxTree& tree, SemanticModel& model, PragmaticModel& pragmatic)
{
	// host-provided interfaces have no declarations, so they only appear in the
	// root namespace; declared ones are found by walking the tree
	assign_namespace_slots(model.get_table().get_root_namespace(), pragmatic);

	interface_slot_visitor slotVisitor(model, pragmatic, m_diagnostics);
	slotVisitor.visit_syntax_tree(tree);

	EscapeAnalyzer escapeAnalyzer(model, pragmatic, m_diagnostics);
	escapeAnalyzer.analyze(tree);

	LayoutGenerator layoutGenerator(model, pragmatic, m_diagnostics);
	layoutGenerator.analyze(tree);
}
