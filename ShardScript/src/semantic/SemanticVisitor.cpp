#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <stdexcept>

using namespace shard;

namespace
{
	gmt::Ref<const SyntaxNode> get_compilation_unit(gmt::Ref<const SyntaxNode> node)
	{
		while (!node.is_null() && node->get_kind() != SyntaxKind::CompilationUnit)
			node = node->get_parent();

		return node;
	}

	bool is_method_kind(const SyntaxKind kind)
	{
		switch (kind)
		{
			case SyntaxKind::FunctionDeclaration:
			case SyntaxKind::ConstructorDeclaration:
			case SyntaxKind::AccessorDeclaration:
			case SyntaxKind::OperatorDeclaration:
				return true;

			default:
				return false;
		}
	}
}

SemanticVisitor::SemanticVisitor(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SyntaxVisitor(), m_table(&model.get_table()), m_diagnostics(diagnostics), m_model(&model) { }

SemanticVisitor::~SemanticVisitor()
{
	while (!m_scopeStack.empty())
	{
		m_scopeStack.pop();
	}
}

SymbolTable& SemanticVisitor::get_symbol_table()
{
	return *m_table;
}

DiagnosticsContext& SemanticVisitor::get_diagnostics()
{
	return m_diagnostics;
}

SemanticModel& SemanticVisitor::get_model()
{
	return *m_model;
}

SemanticScope* SemanticVisitor::get_current_scope()
{
	if (m_scopeStack.empty())
		throw std::runtime_error("scope stack is empty");

	return &m_scopeStack.top();
}

void SemanticVisitor::pop_scope_stack()
{
	m_scopeStack.pop();
}

void SemanticVisitor::push_scope(gmt::Ref<SyntaxSymbol> symbol)
{
	SemanticScope* parent = m_scopeStack.empty() ? nullptr : &m_scopeStack.top();
	m_scopeStack.emplace(symbol, parent);
}

void SemanticVisitor::pop_scope()
{
	m_scopeStack.pop();
}

void SemanticVisitor::declare(gmt::Ref<SyntaxSymbol> symbol)
{
	SemanticScope* scope = get_current_scope();
	scope->declare_symbol(symbol);

	gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
	if (owner.is_null()) // Top-level scopes carry a nullref owner (the compilation unit is not a symbol)
		owner = scope->get_namespace();

	if (!owner.is_null())
		owner->on_symbol_declared(symbol);
}

bool SemanticVisitor::check_name_declared(const std::wstring_view name)
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		if (!scope->find_local(name).is_null())
			return true;
	}

	return false;
}

bool SemanticVisitor::check_name_declared(const gmt::Ref<SyntaxSymbol> symbol)
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> local = scope->find_local(symbol->get_name_reference());
		if (!local.is_null() && local != symbol)
			return true;
	}

	return false;
}

gmt::Ref<SyntaxSymbol> SemanticVisitor::get_owner_symbol()
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		if (owner->is_member() || owner->is_type() || owner->get_kind() == SyntaxKind::NamespaceDirective)
			return owner;
	}

	return gmt::nullref;
}

gmt::Ref<StructSymbol> SemanticVisitor::get_owner_type()
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		if (owner->is_type())
			return owner.as<StructSymbol>();
	}

	return gmt::nullref;
}

gmt::Ref<MethodSymbol> SemanticVisitor::find_host_method_symbol()
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		if (is_method_kind(owner->get_kind()))
			return owner.as<MethodSymbol>();
	}

	return gmt::nullref;
}

bool SemanticVisitor::is_symbol_accessible(gmt::Ref<SyntaxSymbol> symbol, gmt::Ref<const SyntaxNode> symbolDeclaringNode, gmt::Ref<const SyntaxNode> callSiteNode)
{
	if (symbol.is_null())
		return true;

	if (symbol->get_kind() == SyntaxKind::NamespaceDirective)
		return true;

	if (symbol->is_member())
	{
		if (symbol.as<MemberSymbol>()->get_accesibility() == ACS_PUBLIC)
			return true;
	}

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null())
	{
		// unit-level symbols are only reachable from within the same unit
		return true;
	}

	if (parent->get_kind() == SyntaxKind::NamespaceDirective || parent->get_kind() == SyntaxKind::CompilationUnit)
	{
		if (callSiteNode.is_null())
			return false;

		gmt::Ref<const SyntaxNode> declarationFile = get_compilation_unit(symbolDeclaringNode);
		gmt::Ref<const SyntaxNode> currentFile = get_compilation_unit(callSiteNode);
		return !declarationFile.is_null() && declarationFile == currentFile;
	}

	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		if (scope->get_owner() == parent)
			return true;
	}

	return false;
}