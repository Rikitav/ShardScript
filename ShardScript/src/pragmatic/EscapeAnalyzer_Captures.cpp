#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>

#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

using namespace shard;

void EscapeAnalyzer::record_use(gmt::Ref<SyntaxSymbol> symbol)
{
	if (!in_frame())
		return;

	int frameIndex = find_declaring_frame(symbol);
	if (frameIndex < 0)
		return;

	if (frameIndex != static_cast<int>(m_frameStack.size()) - 1)
	{
		// the use belongs to a nested lambda body: the captured reference is
		// handed to the closure at its creation point
		record_capture(symbol);
		return;
	}

	Frame& frame = current_frame();

	bool pastSuspension = frame.seenAwait;

	bool insideSuspendingLoop = false;
	for (int loopId : frame.loopStack)
	{
		if (frame.loopsWithAwait.find(loopId) != frame.loopsWithAwait.end())
		{
			insideSuspendingLoop = true;
			break;
		}
	}

	if (!pastSuspension && !insideSuspendingLoop)
		return;

	add_event(ensure_local_id(symbol), EscapeLevel::GlobalEscape, EscapeChannel::Suspension);
}

gmt::Ref<SyntaxSymbol> EscapeAnalyzer::find_declared_local(std::wstring_view name) const
{
	for (int i = static_cast<int>(m_frameStack.size()) - 1; i >= 0; --i)
	{
		for (gmt::Ref<SyntaxSymbol> symbol : m_frameStack[i].declared)
		{
			if (symbol->get_name() == name)
				return symbol;
		}
	}

	return gmt::Ref<SyntaxSymbol>();
}

void EscapeAnalyzer::record_capture(gmt::Ref<SyntaxSymbol> symbol)
{
	int frameIndex = find_declaring_frame(symbol);

	// every enclosing closure captures the reference transitively
	for (int i = frameIndex + 1; i < static_cast<int>(m_frameStack.size()); ++i)
	{
		gmt::Ref<const ExpressionSyntax> closureSite = m_frameStack[i].closureSite;
		if (closureSite.is_null())
			continue;

		add_edge(ensure_site_id(closureSite), ensure_local_id(symbol));
	}

	// the capture is a use of the capturing closure at its creation point,
	// so suspension liveness chains through it
	Frame& frame = current_frame();

	bool pastSuspension = frame.seenAwait;

	bool insideSuspendingLoop = false;
	for (int loopId : frame.loopStack)
	{
		if (frame.loopsWithAwait.find(loopId) != frame.loopsWithAwait.end())
		{
			insideSuspendingLoop = true;
			break;
		}
	}

	if (frame.closureSite.is_null() || (!pastSuspension && !insideSuspendingLoop))
		return;

	add_event(ensure_site_id(frame.closureSite), EscapeLevel::GlobalEscape, EscapeChannel::Suspension);
}

void EscapeAnalyzer::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	register_allocation_site(node);

	// the anonymous method symbol is bound to the lambda's parameters list node
	gmt::Ref<MethodSymbol> symbol;
	if (!node->get_parameters().is_null())
	{
		auto paramsRef = ref_from(node->get_parameters().as_ptr());
		if (paramsRef.has_value())
		{
			gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*paramsRef);
			if (!bound.is_null() && bound->is_method())
				symbol = bound.as<MethodSymbol>();
		}
	}

	gmt::Ref<const ExpressionSyntax> closureSite;
	auto nodeRef = ref_from(node);
	if (nodeRef.has_value())
		closureSite = *nodeRef;

	enter_frame(symbol, closureSite);
	declare_parameters(symbol);

	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (!body.is_null())
		body.as_ptr()->accept(*this);

	leave_frame();
}

void EscapeAnalyzer::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> symbol = get_symbol_table().lookup_symbol(*nodeRef);
	if (symbol.is_null())
		return;

	SyntaxKind kind = symbol->get_kind();
	if (kind == SyntaxKind::VariableStatement || kind == SyntaxKind::Parameter)
		record_use(symbol);

	SyntaxVisitor::visit_identifier_name_type(node);
}

void EscapeAnalyzer::visit_for_in_statement(const ForInStatementSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (nodeRef.has_value())
		declare_local(lookup_symbol<VariableSymbol>(*nodeRef));

	bool pushed = in_frame();
	if (pushed)
		current_frame().loopStack.push_back(m_nextLoopId++);

	SyntaxVisitor::visit_for_in_statement(node);

	if (pushed)
		current_frame().loopStack.pop_back();
}

void EscapeAnalyzer::visit_catch_clause(const CatchClauseSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (nodeRef.has_value())
		declare_local(lookup_symbol<VariableSymbol>(*nodeRef));

	// upward flows out of an exceptional path are cold (spec 4.6); the
	// exception variable declaration itself is not an exposure, so it
	// stays outside the cold region
	++m_coldDepth;
	SyntaxVisitor::visit_catch_clause(node);
	--m_coldDepth;
}
