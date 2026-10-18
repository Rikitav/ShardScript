#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>

#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>

#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

using namespace shard;

namespace
{
	bool is_assignment_operator(TokenType type)
	{
		switch (type)
		{
		case TokenType::AssignOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
			return true;

		default:
			return false;
		}
	}
}

// collects the locals and allocation sites an expression directly refers to;
// descends through combinators but stops at allocation sites (their inner
// flows are bound to the container) and at lambda bodies (captured uses)
class EscapeAnalyzer::reference_collector final : public SyntaxVisitor
{
	SymbolTable& m_table;
	gmt::Arena& m_arena;
	ExpressionRefs& m_refs;
	bool m_includeReceivers;

public:
	reference_collector(SymbolTable& table, gmt::Arena& arena, ExpressionRefs& refs, bool includeReceivers)
		: m_table(table)
		, m_arena(arena)
		, m_refs(refs)
		, m_includeReceivers(includeReceivers)
	{ }

	void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override
	{
		if (node == nullptr)
			return;

		auto nodeRef = m_arena.ref_from_ptr(node);
		if (!nodeRef.has_value())
			return;

		gmt::Ref<SyntaxSymbol> symbol = m_table.lookup_symbol(*nodeRef);
		if (symbol.is_null())
			return;

		SyntaxKind kind = symbol->get_kind();
		if (kind == SyntaxKind::VariableStatement || kind == SyntaxKind::Parameter)
			m_refs.locals.get_vector().push_back(symbol);
	}

	void visit_member_access_expression(const MemberAccessExpressionSyntax* node) override
	{
		if (node == nullptr || !m_includeReceivers)
			return;

		if (!node->get_previous().is_null())
			node->get_previous().as_ptr()->accept(*this);
	}

	void visit_indexator_expression(const IndexatorExpressionSyntax* node) override
	{
		if (node == nullptr)
			return;

		// index arguments are reads and always collected; the receiver is
		// only collected when the chain itself is passed somewhere
		if (m_includeReceivers && !node->get_previous().is_null())
			node->get_previous().as_ptr()->accept(*this);

		if (!node->get_arguments().is_null())
			node->get_arguments().as_ptr()->accept(*this);
	}

	void visit_object_creation_expression(const ObjectExpressionSyntax* node) override
	{
		if (node == nullptr)
			return;

		auto nodeRef = m_arena.ref_from_ptr(node);
		if (nodeRef.has_value())
			m_refs.sites.get_vector().push_back(*nodeRef);
	}

	void visit_collection_expression(const CollectionExpressionSyntax* node) override
	{
		if (node == nullptr)
			return;

		auto nodeRef = m_arena.ref_from_ptr(node);
		if (nodeRef.has_value())
			m_refs.sites.get_vector().push_back(*nodeRef);
	}

	void visit_lambda_expression(const LambdaExpressionSyntax* node) override
	{
		if (node == nullptr)
			return;

		auto nodeRef = m_arena.ref_from_ptr(node);
		if (nodeRef.has_value())
			m_refs.sites.get_vector().push_back(*nodeRef);
	}
};

EscapeAnalyzer::EscapeAnalyzer(SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
	, m_pragmatic(pragmatic)
{
}

void EscapeAnalyzer::clear_graph()
{
	m_graph = Graph();
	m_frameStack.clear();
	m_walked.clear();
	m_scalarCache.clear();
	m_coldPoints.clear();
	m_nextLoopId = 0;
	m_inDefer = 0;
	m_coldDepth = 0;
}

int EscapeAnalyzer::ensure_local_id(gmt::Ref<SyntaxSymbol> symbol)
{
	auto it = m_graph.localIds.find(symbol);
	if (it != m_graph.localIds.end())
		return it->second;

	int id = m_graph.nextId++;
	m_graph.localIds[symbol] = id;
	m_graph.levels.push_back(EscapeLevel::NoEscape);
	m_graph.channelSets.push_back(EscapeChannel::None);
	m_graph.coldChannels.push_back(EscapeChannel::None);
	m_graph.identityExposed.push_back(false);
	m_graph.idOwners.push_back(in_frame() ? current_frame().method : gmt::Ref<MethodSymbol>());

	return id;
}

int EscapeAnalyzer::ensure_site_id(gmt::Ref<const ExpressionSyntax> site)
{
	auto it = m_graph.siteIds.find(site);
	if (it != m_graph.siteIds.end())
		return it->second;

	int id = m_graph.nextId++;
	m_graph.siteIds[site] = id;
	m_graph.levels.push_back(EscapeLevel::NoEscape);
	m_graph.channelSets.push_back(EscapeChannel::None);
	m_graph.coldChannels.push_back(EscapeChannel::None);
	m_graph.identityExposed.push_back(false);
	m_graph.idOwners.push_back(in_frame() ? current_frame().method : gmt::Ref<MethodSymbol>());

	return id;
}

void EscapeAnalyzer::add_edge(int from, int to)
{
	m_graph.edges.emplace_back(from, to);
}

void EscapeAnalyzer::add_event(int id, EscapeLevel level, EscapeChannel channel, bool cold)
{
	m_graph.events.push_back({ id, level, channel, cold || m_coldDepth > 0 });
}

bool EscapeAnalyzer::join_node(int id, EscapeLevel level, EscapeChannel channel)
{
	bool changed = false;

	if (level > m_graph.levels[id])
	{
		m_graph.levels[id] = level;
		changed = true;
	}

	EscapeChannel merged = m_graph.channelSets[id] | channel;
	if (merged != m_graph.channelSets[id])
	{
		m_graph.channelSets[id] = merged;
		changed = true;
	}

	return changed;
}

bool EscapeAnalyzer::join_cold(int id, EscapeChannel channel)
{
	EscapeChannel merged = m_graph.coldChannels[id] | channel;
	if (merged == m_graph.coldChannels[id])
		return false;

	m_graph.coldChannels[id] = merged;
	return true;
}

void EscapeAnalyzer::enter_frame(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> closureSite)
{
	Frame frame;
	frame.method = method;
	frame.closureSite = closureSite;
	m_frameStack.push_back(std::move(frame));
}

void EscapeAnalyzer::leave_frame()
{
	if (m_frameStack.empty())
		return;

	Frame frame = std::move(m_frameStack.back());
	m_frameStack.pop_back();

	if (!frame.method.is_null())
	{
		m_pragmatic.set_local_order(frame.method, frame.orderedLocals);

		WalkedMethod walked;
		walked.method = frame.method;
		walked.returnedExpressions = std::move(frame.returnedExpressions);
		m_walked.push_back(std::move(walked));
	}
}

bool EscapeAnalyzer::in_frame() const
{
	return !m_frameStack.empty();
}

EscapeAnalyzer::Frame& EscapeAnalyzer::current_frame()
{
	return m_frameStack.back();
}

void EscapeAnalyzer::declare_local(gmt::Ref<SyntaxSymbol> symbol)
{
	if (symbol.is_null() || !in_frame())
		return;

	if (current_frame().declared.insert(symbol).second)
		current_frame().orderedLocals.push_back(symbol);

	ensure_local_id(symbol);
}

int EscapeAnalyzer::find_declaring_frame(gmt::Ref<SyntaxSymbol> symbol) const
{
	for (int i = static_cast<int>(m_frameStack.size()) - 1; i >= 0; --i)
	{
		if (m_frameStack[i].declared.find(symbol) != m_frameStack[i].declared.end())
			return i;
	}

	return -1;
}

void EscapeAnalyzer::collect_expression_refs(gmt::Ref<const ExpressionSyntax> expression, ExpressionRefs& refs, bool includeReceivers)
{
	if (expression.is_null())
		return;

	reference_collector collector(get_symbol_table(), get_model().get_tree().get_arena(), refs, includeReceivers);
	expression.as_ptr()->accept(collector);
}

void EscapeAnalyzer::expose_expression(gmt::Ref<const ExpressionSyntax> expression, EscapeLevel level, EscapeChannel channel, bool includeReceivers)
{
	if (m_inDefer > 0)
		return;

	ExpressionRefs refs;
	collect_expression_refs(expression, refs, includeReceivers);

	bool isCold = m_coldDepth > 0;

	for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
	{
		int id = ensure_local_id(local);
		add_event(id, level, channel);

		if (isCold)
			record_cold_point(id, expression);
	}

	for (gmt::Ref<const ExpressionSyntax> site : refs.sites.get_vector())
	{
		int id = ensure_site_id(site);
		add_event(id, level, channel);

		if (isCold)
			record_cold_point(id, expression);
	}
}

bool EscapeAnalyzer::is_fresh_invokation(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null() || expression->get_kind() != SyntaxKind::InvokationExpression)
		return false;

	gmt::Ref<MethodSymbol> callee;
	if (!expression.is_null())
	{
		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(expression);
		if (!bound.is_null() && bound->is_method())
			callee = bound.as<MethodSymbol>();
	}

	if (callee.is_null())
		return false;

	// a Fresh-summary callee returns a value it created: its arguments do
	// not flow into the caller's return slot (2.1e chaining)
	EscapeSummary summary = m_pragmatic.get_summary(callee);
	return summary.computed && summary.returnDisposition == ReturnDisposition::Fresh;
}

void EscapeAnalyzer::mark_identity(gmt::Ref<const ExpressionSyntax> expression, bool includeReceivers)
{
	if (expression.is_null())
		return;

	ExpressionRefs refs;
	collect_expression_refs(expression, refs, includeReceivers);

	for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
		m_graph.identityExposed[ensure_local_id(local)] = true;

	for (gmt::Ref<const ExpressionSyntax> site : refs.sites.get_vector())
		m_graph.identityExposed[ensure_site_id(site)] = true;
}

void EscapeAnalyzer::record_call_exposure(gmt::Ref<const ExpressionSyntax> expression, gmt::Ref<MethodSymbol> callee, std::uint16_t parameterIndex, bool receiver)
{
	if (expression.is_null() || callee.is_null())
		return;

	ExpressionRefs refs;
	collect_expression_refs(expression, refs, receiver);

	bool cold = m_coldDepth > 0;

	for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
		m_graph.callExposures.push_back({ ensure_local_id(local), callee, parameterIndex, receiver, cold });

	for (gmt::Ref<const ExpressionSyntax> site : refs.sites.get_vector())
		m_graph.callExposures.push_back({ ensure_site_id(site), callee, parameterIndex, receiver, cold });
}

void EscapeAnalyzer::alias_into(gmt::Ref<SyntaxSymbol> target, gmt::Ref<const ExpressionSyntax> sourceExpression)
{
	if (target.is_null() || sourceExpression.is_null())
		return;

	ExpressionRefs refs;
	collect_expression_refs(sourceExpression, refs, false);

	int targetId = ensure_local_id(target);

	for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
		add_edge(ensure_local_id(local), targetId);

	// the target names every allocation site of the source expression:
	// an escaping target takes the site with it
	for (gmt::Ref<const ExpressionSyntax> site : refs.sites.get_vector())
		add_edge(targetId, ensure_site_id(site));
}

void EscapeAnalyzer::register_allocation_site(const ExpressionSyntax* site)
{
	if (site == nullptr)
		return;

	auto siteRef = ref_from(site);
	if (siteRef.has_value())
		ensure_site_id(*siteRef);
}

void EscapeAnalyzer::declare_parameters(gmt::Ref<MethodSymbol> method)
{
	if (method.is_null())
		return;

	for (const gmt::Ref<ParameterSymbol>& parameter : method->get_parameters())
		declare_local(parameter);
}

void EscapeAnalyzer::analyze_member_body(gmt::Ref<MethodSymbol> method, gmt::Ref<const BlockSyntax> body)
{
	if (body.is_null())
		return;

	enter_frame(method, gmt::Ref<const ExpressionSyntax>());
	declare_parameters(method);

	body.as_ptr()->accept(*this);

	leave_frame();
}

void EscapeAnalyzer::run_graph_fixpoint()
{
	bool changed = true;
	while (changed)
	{
		changed = false;

		for (const std::pair<int, int>& edge : m_graph.edges)
		{
			changed |= join_node(edge.second, m_graph.levels[edge.first], m_graph.channelSets[edge.first]);

			// identity is shared along alias edges: a copy of an
			// identity-exposed value is identity-exposed itself
			if (m_graph.identityExposed[edge.first] && !m_graph.identityExposed[edge.second])
			{
				m_graph.identityExposed[edge.second] = true;
				changed = true;
			}

			// cold channels propagate the same way: an escaping alias takes
			// the cold flows of whatever it names along
			changed |= join_cold(edge.second, m_graph.coldChannels[edge.first]);
		}

		for (const Graph::Event& event : m_graph.events)
		{
			if (event.cold)
				changed |= join_cold(event.id, event.channel);
			else
				changed |= join_node(event.id, event.level, event.channel);
		}
	}
}

void EscapeAnalyzer::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	analyze_member_body(symbol, node->get_body());
}

void EscapeAnalyzer::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	analyze_member_body(symbol, node->get_body());
}

void EscapeAnalyzer::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	analyze_member_body(symbol, node->get_body());
}

void EscapeAnalyzer::visit_variable_statement(const VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	SyntaxVisitor::visit_variable_statement(node);

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> variable = lookup_symbol<VariableSymbol>(*nodeRef);
	if (variable.is_null())
		return;

	declare_local(variable);

	if (!node->get_expression().is_null())
		alias_into(variable, node->get_expression());
}

void EscapeAnalyzer::visit_return_statement(const ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (m_inDefer == 0 && !node->get_expression().is_null())
	{
		if (!is_fresh_invokation(node->get_expression()))
			expose_expression(node->get_expression(), EscapeLevel::GlobalEscape, EscapeChannel::Return);

		if (in_frame())
			current_frame().returnedExpressions.push_back(node->get_expression());
	}

	SyntaxVisitor::visit_return_statement(node);
}

void EscapeAnalyzer::visit_throw_statement(const ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		expose_expression(node->get_expression(), EscapeLevel::GlobalEscape, EscapeChannel::Exception);

	SyntaxVisitor::visit_throw_statement(node);
}

void EscapeAnalyzer::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	if (node == nullptr)
		return;

	SyntaxVisitor::visit_arrow_clause(node);

	if (m_inDefer == 0 && !node->get_expression().is_null())
	{
		if (!is_fresh_invokation(node->get_expression()))
			expose_expression(node->get_expression(), EscapeLevel::GlobalEscape, EscapeChannel::Return);

		if (in_frame())
			current_frame().returnedExpressions.push_back(node->get_expression());
	}
}

void EscapeAnalyzer::visit_binary_expression(const BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	TokenType operatorType = node->get_operator_token().get_type();
	gmt::Ref<const ExpressionSyntax> left = node->get_left();
	gmt::Ref<const ExpressionSyntax> right = node->get_right();

	if (operatorType == TokenType::AssignOperator && !left.is_null() && !right.is_null())
	{
		if (left->get_kind() == SyntaxKind::IdentifierNameType)
		{
			gmt::Ref<SyntaxSymbol> symbol = get_symbol_table().lookup_symbol(left);

			bool isLocal = !symbol.is_null();
			isLocal = isLocal && symbol->get_kind() == SyntaxKind::VariableStatement;
			bool isParameter = !symbol.is_null();
			isParameter = isParameter && symbol->get_kind() == SyntaxKind::Parameter;

			if (isLocal || isParameter)
				alias_into(symbol, right);
			else
				expose_expression(right, EscapeLevel::GlobalEscape, EscapeChannel::Store);
		}
		else
		{
			// a store into a field, property or element (E2): the stored reference outlives the frame
			expose_expression(right, EscapeLevel::GlobalEscape, EscapeChannel::Store);
		}
	}
	else if (is_assignment_operator(operatorType) && !right.is_null())
	{
		expose_expression(right, EscapeLevel::GlobalEscape, EscapeChannel::Store);
	}

	SyntaxVisitor::visit_binary_expression(node);
}

void EscapeAnalyzer::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (m_inDefer == 0)
	{
		gmt::Ref<MethodSymbol> callee;
		auto nodeRef = ref_from(node);
		if (nodeRef.has_value())
		{
			gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
			if (!bound.is_null() && bound->is_method())
				callee = bound.as<MethodSymbol>();
		}

		// a statically known, concrete callee only exposes its arguments downward (ArgEscape);
		// anything dispatched or unknown is an identity use escaping upward (E5)
		bool calleeIsInterfaceMember = false;
		if (!callee.is_null() && !callee->get_parent().is_null())
			calleeIsInterfaceMember = callee->get_parent()->get_kind() == SyntaxKind::InterfaceDeclaration;

		bool knownConcrete = !callee.is_null();
		knownConcrete = knownConcrete && !callee->get_is_virtual();
		knownConcrete = knownConcrete && !calleeIsInterfaceMember;

		EscapeLevel argumentLevel = knownConcrete ? EscapeLevel::ArgEscape : EscapeLevel::GlobalEscape;

		bool instanceCallee = !callee.is_null();
		instanceCallee = instanceCallee && callee->get_linking() == LINK_INSTANCE;

		gmt::Ref<const ExpressionSyntax> receiver = node->get_previous();
		if (!receiver.is_null())
		{
			expose_expression(receiver, argumentLevel, EscapeChannel::UnknownCallee, true);

			if (knownConcrete)
			{
				if (instanceCallee)
					record_call_exposure(receiver, callee, 0, true);
			}
			else
			{
				mark_identity(receiver, true);
			}
		}
		else
		{
			// a bare identifier call invokes a local (lambda or delegate variable);
			// the identifier is a token, not a receiver expression
			gmt::Ref<SyntaxSymbol> local = find_declared_local(node->get_identifier().get_lexeme());
			if (!local.is_null())
			{
				add_event(ensure_local_id(local), argumentLevel, EscapeChannel::UnknownCallee);

				if (!knownConcrete)
					m_graph.identityExposed[ensure_local_id(local)] = true;
			}
		}

		gmt::Ref<const ArgumentsListSyntax> arguments = node->get_arguments();
		if (!arguments.is_null())
		{
			const std::uint16_t parameterOffset = instanceCallee ? 1 : 0;
			const auto argumentNodes = arguments->get_arguments().as_span();

			for (std::size_t i = 0; i < argumentNodes.size(); ++i)
			{
				const gmt::Ref<ArgumentSyntax>& argument = argumentNodes[i];
				if (argument.is_null())
					continue;

				gmt::Ref<const ExpressionSyntax> argumentExpression = argument->get_expression();
				expose_expression(argumentExpression, argumentLevel, EscapeChannel::UnknownCallee);

				if (knownConcrete)
					record_call_exposure(argumentExpression, callee, parameterOffset + static_cast<std::uint16_t>(i), false);
				else
					mark_identity(argumentExpression, false);
			}
		}
	}

	SyntaxVisitor::visit_invokation_expression(node);
}

void EscapeAnalyzer::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);

	bool isSite = true;
	if (!node->is_array_creation() && nodeRef.has_value())
	{
		gmt::Ref<StructSymbol> type = get_model().get_expression_type(*nodeRef);
		if (!type.is_null() && get_model().is_primitive_type(type))
			isSite = false;
	}

	if (isSite)
		register_allocation_site(node);

	if (m_inDefer == 0)
	{
		bool hasKnownConstructor = false;
		gmt::Ref<ConstructorSymbol> constructor;
		if (nodeRef.has_value())
			constructor = lookup_symbol<ConstructorSymbol>(*nodeRef);

		hasKnownConstructor = !constructor.is_null();

		gmt::Ref<const ArgumentsListSyntax> arguments = node->get_arguments();
		if (!arguments.is_null())
		{
			int siteId = isSite && nodeRef.has_value() ? ensure_site_id(*nodeRef) : -1;

			const bool instanceConstructor = hasKnownConstructor && constructor->get_linking() == LINK_INSTANCE;
			const std::uint16_t parameterOffset = instanceConstructor ? 1 : 0;
			const auto argumentNodes = arguments->get_arguments().as_span();

			for (std::size_t i = 0; i < argumentNodes.size(); ++i)
			{
				const gmt::Ref<ArgumentSyntax>& argument = argumentNodes[i];
				if (argument.is_null())
					continue;

				gmt::Ref<const ExpressionSyntax> argumentExpression = argument->get_expression();

				// constructor arguments are stored into the new object (E2): they escape wherever the object escapes
				if (siteId >= 0 && !argumentExpression.is_null())
				{
					ExpressionRefs refs;
					collect_expression_refs(argumentExpression, refs, false);

					for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
						add_edge(siteId, ensure_local_id(local));

					for (gmt::Ref<const ExpressionSyntax> argumentSite : refs.sites.get_vector())
						add_edge(siteId, ensure_site_id(argumentSite));
				}

				EscapeLevel argumentLevel = hasKnownConstructor ? EscapeLevel::ArgEscape : EscapeLevel::GlobalEscape;
				expose_expression(argumentExpression, argumentLevel, EscapeChannel::UnknownCallee);

				if (hasKnownConstructor)
					record_call_exposure(argumentExpression, constructor, parameterOffset + static_cast<std::uint16_t>(i), false);
			}
		}
	}

	SyntaxVisitor::visit_object_creation_expression(node);
}

void EscapeAnalyzer::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	register_allocation_site(node);

	if (m_inDefer == 0)
	{
		auto nodeRef = ref_from(node);
		int siteId = nodeRef.has_value() ? ensure_site_id(*nodeRef) : -1;

		// elements are stored into the collection: they escape where it escapes
		for (const gmt::Ref<ExpressionSyntax>& value : node->get_values().as_span())
		{
			if (value.is_null() || siteId < 0)
				continue;

			ExpressionRefs refs;
			collect_expression_refs(value, refs, false);

			for (gmt::Ref<SyntaxSymbol> local : refs.locals.get_vector())
				add_edge(siteId, ensure_local_id(local));

			for (gmt::Ref<const ExpressionSyntax> elementSite : refs.sites.get_vector())
				add_edge(siteId, ensure_site_id(elementSite));
		}
	}

	SyntaxVisitor::visit_collection_expression(node);
}

void EscapeAnalyzer::visit_defer_statement(const DeferStatementSyntax* node)
{
	if (node == nullptr)
		return;

	// defer bodies run at scope exit, still within the frame: nothing they touch escapes through them
	++m_inDefer;
	SyntaxVisitor::visit_defer_statement(node);
	--m_inDefer;
}

void EscapeAnalyzer::visit_defer_block(const DeferBlockSyntax* node)
{
	if (node == nullptr)
		return;

	++m_inDefer;
	SyntaxVisitor::visit_defer_block(node);
	--m_inDefer;
}

void EscapeAnalyzer::visit_defer_variable(const DeferVariableSyntax* node)
{
	if (node == nullptr)
		return;

	++m_inDefer;
	SyntaxVisitor::visit_defer_variable(node);
	--m_inDefer;
}

void EscapeAnalyzer::visit_is_expression(const IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// a type test observes the operand's identity without escaping it
	if (m_inDefer == 0)
		mark_identity(node->get_expression(), false);

	SyntaxVisitor::visit_is_expression(node);
}

void EscapeAnalyzer::visit_cast_expression(const CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (m_inDefer == 0)
		mark_identity(node->get_expression(), false);

	SyntaxVisitor::visit_cast_expression(node);
}
