#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

#include <stdexcept>

using namespace shard;

void EscapeAnalyzer::analyze(SyntaxTree& tree)
{
	// the walk consults committed callee summaries (is_fresh_invokation), so
	// the graph is rebuilt per iteration; every update is a monotone join and
	// the loop terminates once committed summaries stabilize. The graph itself
	// is a deterministic function of the tree and the committed summaries, so
	// a stable commit implies a stable graph
	for (int iteration = 0; iteration < 64; ++iteration)
	{
		clear_graph();
		visit_syntax_tree(tree);
		run_graph_fixpoint();

		gmt::unordered_map<MethodSymbol*, EscapeSummary> summaries = derive_summaries();
		apply_call_exposures(summaries);
		run_graph_fixpoint();

		if (!commit_summaries(summaries))
		{
			assign_verdicts(summaries);
			return;
		}
	}

	throw std::runtime_error("pragmatic escape analysis did not reach a stable fixpoint");
}

ParameterEscapeSummary EscapeAnalyzer::derive_parameter_summary(MethodSymbol* method, ParameterSymbol* parameter, bool isReceiver)
{
	ParameterEscapeSummary summary;

	int id = -1;

	if (!isReceiver)
	{
		auto it = m_graph.localIds.find(parameter);
		if (it != m_graph.localIds.end())
			id = it->second;
	}
	else
	{
		// the binder declares 'this' ad-hoc in the callee scope, so the
		// symbol is absent from get_parameters(): recover its graph id by
		// owner and name
		for (const auto& pair : m_graph.localIds)
		{
			bool ownedHere = m_graph.idOwners[pair.second] == method;
			bool isThisParameter = pair.first->get_kind() == SyntaxKind::Parameter;
			isThisParameter = isThisParameter && pair.first->get_name() == L"this";

			if (ownedHere && isThisParameter)
			{
				id = pair.second;
				break;
			}
		}
	}

	if (id < 0)
		return summary;

	summary.channels = m_graph.channelSets[id];
	summary.coldChannels = m_graph.coldChannels[id];

	const EscapeChannel upward = EscapeChannel::Return | EscapeChannel::Store | EscapeChannel::Capture | EscapeChannel::Exception;
	bool hasUpwardChannel = (summary.channels & upward) != EscapeChannel::None;

	summary.escapes = m_graph.levels[id] == EscapeLevel::GlobalEscape || hasUpwardChannel;

	return summary;
}

gmt::unordered_map<MethodSymbol*, EscapeSummary> EscapeAnalyzer::derive_summaries()
{
	gmt::unordered_map<MethodSymbol*, EscapeSummary> result;

	for (const WalkedMethod& walked : m_walked)
	{
		MethodSymbol* method = walked.method;
		if ((method == nullptr) || result.find(method) != result.end())
			continue;

		EscapeSummary summary;
		summary.computed = true;
		summary.returnType = method->get_return_type();

		if (method->get_linking() == LINK_INSTANCE)
			summary.parameters.push_back(derive_parameter_summary(method, nullptr, true));

		for (ParameterSymbol* parameter : method->get_parameters())
			summary.parameters.push_back(derive_parameter_summary(method, parameter, false));

		result[method] = summary;
	}

	for (const WalkedMethod& walked : m_walked)
	{
		if ((walked.method == nullptr))
			continue;

		result[walked.method].returnDisposition = classify_returns(walked, result);
	}

	return result;
}

void EscapeAnalyzer::apply_call_exposures(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	for (const Graph::CallExposure& exposure : m_graph.callExposures)
	{
		auto it = summaries.find(exposure.callee);

		// a callee with no derived summary was never walked in this tree
		// (libfunc, bodyless member): the trusted conservative default
		if (it == summaries.end() || !it->second.computed)
		{
			add_event(exposure.nodeId, EscapeLevel::GlobalEscape, EscapeChannel::UnknownCallee, exposure.cold);
			continue;
		}

		const EscapeSummary& summary = it->second;
		if (exposure.parameterIndex >= summary.parameters.size())
		{
			// arity mismatch: the binder resolved an overload the summary does not cover
			add_event(exposure.nodeId, EscapeLevel::GlobalEscape, EscapeChannel::UnknownCallee, exposure.cold);
			continue;
		}

		const ParameterEscapeSummary& parameter = summary.parameters[exposure.parameterIndex];
		if (parameter.escapes)
			add_event(exposure.nodeId, EscapeLevel::GlobalEscape, parameter.channels, exposure.cold);

		// a cold flow inside the callee stays cold at the caller's call site
		if (parameter.coldChannels != EscapeChannel::None)
			add_event(exposure.nodeId, EscapeLevel::GlobalEscape, parameter.coldChannels, true);
	}
}

bool EscapeAnalyzer::commit_summaries(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	bool changed = false;

	for (const auto& pair : summaries)
	{
		EscapeSummary previous = m_pragmatic.get_summary(pair.first);

		bool same = previous.computed == pair.second.computed;
		same = same && previous.returnDisposition == pair.second.returnDisposition;
		same = same && previous.parameters.size() == pair.second.parameters.size();
		same = same && previous.returnType == pair.second.returnType;

		if (same)
		{
			for (std::size_t i = 0; i < pair.second.parameters.size(); ++i)
			{
				same = same && previous.parameters[i].channels == pair.second.parameters[i].channels;
				same = same && previous.parameters[i].coldChannels == pair.second.parameters[i].coldChannels;
				same = same && previous.parameters[i].escapes == pair.second.parameters[i].escapes;
			}
		}

		if (!same)
		{
			m_pragmatic.set_summary(pair.first, pair.second);
			changed = true;
		}
	}

	return changed;
}

int EscapeAnalyzer::classify_local_roots(MethodSymbol* method, SyntaxSymbol* local)
{
	auto startIt = m_graph.localIds.find(local);
	if (startIt == m_graph.localIds.end())
		return 0;

	const EscapeChannel taint = EscapeChannel::Store | EscapeChannel::Capture | EscapeChannel::Suspension | EscapeChannel::Exception;

	int mark = 0;
	gmt::unordered_set<int> visited;
	gmt::inline_vector<int, 16> stack;
	stack.push_back(startIt->second);

	while (!stack.get_vector().empty())
	{
		int id = stack.get_vector().back();
		stack.get_vector().pop_back();

		// an alias cycle never resolves to a clean fresh/parameter root
		if (visited.find(id) != visited.end())
			return 0;

		visited.insert(id);

		if (!(m_graph.idOwners[id] == method))
			return 0;

		if ((m_graph.channelSets[id] & taint) != EscapeChannel::None)
			return 0;

		bool escapedToUnknown = m_graph.levels[id] == EscapeLevel::GlobalEscape;
		escapedToUnknown = escapedToUnknown && (m_graph.channelSets[id] & EscapeChannel::UnknownCallee) != EscapeChannel::None;
		if (escapedToUnknown)
			return 0;

		bool isSite = false;
		for (const auto& pair : m_graph.siteIds)
		{
			if (pair.second == id)
			{
				isSite = true;
				break;
			}
		}

		if (isSite)
		{
			mark |= 1; // fresh site created in this method
			continue;
		}

		bool isParameter = local->get_kind() == SyntaxKind::Parameter;
		if (id == startIt->second && isParameter)
		{
			mark |= 2; // the alias chain is rooted in a parameter
			continue;
		}

		for (const std::pair<int, int>& edge : m_graph.edges)
		{
			if (edge.first == id)
				stack.push_back(edge.second);
		}
	}

	return mark;
}

int EscapeAnalyzer::classify_return_expression(MethodSymbol* method, const ExpressionSyntax* expression, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	ExpressionRefs refs;
	collect_expression_refs(expression, refs, false);

	int mark = 0;

	for (const ExpressionSyntax* site : refs.sites.get_vector())
	{
		auto it = m_graph.siteIds.find(site);
		if (it == m_graph.siteIds.end())
			return 0;

		if (!(m_graph.idOwners[it->second] == method))
			return 0;

		mark |= 1;
	}

	for (SyntaxSymbol* local : refs.locals.get_vector())
	{
		int localMark = classify_local_roots(method, local);
		if (localMark == 0)
			return 0;

		mark |= localMark;
	}

	if (mark == 0 && expression->get_kind() == SyntaxKind::InvokationExpression)
	{
		// transitive return-slot chaining (2.1e): a call to a Fresh-summary
		// callee hands the caller a caller-placeable value without a local site
		MethodSymbol* callee = nullptr;
		if (!(expression == nullptr))
		{
			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(expression);
			if (!(bound == nullptr) && bound->is_method())
				callee = static_cast<MethodSymbol*>(bound);
		}

		if (!(callee == nullptr))
		{
			auto it = summaries.find(callee);
			if (it != summaries.end() && it->second.returnDisposition == ReturnDisposition::Fresh)
				mark = 1;
		}
	}

	return mark;
}

ReturnDisposition EscapeAnalyzer::classify_returns(const WalkedMethod& walked, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	if (walked.returnedExpressions.empty())
		return ReturnDisposition::Unknown;

	int combinedMark = 0;
	int contributions = 0;

	for (const ExpressionSyntax* expression : walked.returnedExpressions)
	{
		int mark = classify_return_expression(walked.method, expression, summaries);
		if (mark == 0)
			continue; // non-reference returns (literals) do not constrain the disposition

		combinedMark |= mark;
		++contributions;
	}

	if (contributions == 0)
		return ReturnDisposition::Unknown;

	if (combinedMark == 1)
		return ReturnDisposition::Fresh;

	if (combinedMark == 2)
		return ReturnDisposition::ReturnsParameter;

	return ReturnDisposition::Unknown;
}

bool EscapeAnalyzer::is_caller_placeable(MethodSymbol* owner) const
{
	if ((owner == nullptr))
		return false;

	StructSymbol* returnType = owner->get_return_type();
	if ((returnType == nullptr))
		return false;

	// concrete types only: interfaces (unknown size + vtable identity),
	// nullables, arrays and type parameters stay heap; a generic closed over
	// concrete arguments has a fixed shape and chains like any struct
	if (returnType->get_kind() == SyntaxKind::StructDeclaration)
		return true;

	if (returnType->get_kind() != SyntaxKind::GenericType)
		return false;

	return static_cast<GenericTypeSymbol*>(returnType)->get_underlaying_type()->get_kind() == SyntaxKind::StructDeclaration;
}

bool EscapeAnalyzer::cone_closed(int id, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries) const
{
	for (const Graph::CallExposure& exposure : m_graph.callExposures)
	{
		if (exposure.nodeId != id)
			continue;

		auto it = summaries.find(exposure.callee);
		if (it == summaries.end() || !it->second.computed)
			return false;
	}

	return true;
}

bool EscapeAnalyzer::participates_in_reference_cycle(StructSymbol* type)
{
	gmt::unordered_set<StructSymbol*> visited;
	gmt::inline_vector<StructSymbol*, 8> stack;
	stack.push_back(type);

	while (!stack.get_vector().empty())
	{
		StructSymbol* current = stack.get_vector().back();
		stack.get_vector().pop_back();

		for (FieldSymbol* field : current->fields)
		{
			StructSymbol* fieldType = field->get_return_type();
			if ((fieldType == nullptr) || fieldType->get_kind() != SyntaxKind::StructDeclaration)
				continue;

			if (fieldType == type)
				return true;

			if (visited.insert(fieldType).second)
				stack.push_back(fieldType);
		}
	}

	return false;
}

bool EscapeAnalyzer::is_scalar_eligible(StructSymbol* type)
{
	if ((type == nullptr))
		return false;

	if (type->get_kind() != SyntaxKind::StructDeclaration)
		return false;

	auto cached = m_scalarCache.find(type);
	if (cached != m_scalarCache.end())
		return cached->second;

	bool eligible = true;

	for (FieldSymbol* field : type->fields)
	{
		StructSymbol* fieldType = field->get_return_type();
		bool nullableField = !(fieldType == nullptr);
		nullableField = nullableField && fieldType->get_kind() == SyntaxKind::NullableType;

		if (nullableField)
		{
			eligible = false;
			break;
		}
	}

	if (eligible)
		eligible = !participates_in_reference_cycle(type);

	m_scalarCache[type] = eligible;
	return eligible;
}

AllocationSiteVerdict EscapeAnalyzer::classify_node(int id, StructSymbol* nodeType, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	AllocationSiteVerdict verdict;
	verdict.level = m_graph.levels[id];
	verdict.channels = m_graph.channelSets[id];

	if (verdict.level == EscapeLevel::GlobalEscape)
	{
		bool returnOnly = verdict.channels == EscapeChannel::Return;
		verdict.verdict = returnOnly && is_caller_placeable(m_graph.idOwners[id]) ? PlacementVerdict::StackReturn : PlacementVerdict::Heap;
	}
	else if (verdict.level == EscapeLevel::ArgEscape)
	{
		bool closed = cone_closed(id, summaries);
		bool hasCold = m_graph.coldChannels[id] != EscapeChannel::None;

		// cone-closed ArgEscape with cold channels: the cone adds no hot
		// escape, so the site is born on the stack and migrates on the
		// cold path like any other Promote
		if (closed && hasCold)
			verdict.verdict = PlacementVerdict::Promote;
		else
			verdict.verdict = closed ? PlacementVerdict::Stack : PlacementVerdict::Heap;
	}
	else if (m_graph.coldChannels[id] != EscapeChannel::None)
	{
		// hot NoEscape with cold-only upward flows: born on the stack,
		// migrated at the cold escape point (materialization-on-merge
		// would be needed for Scalar, so Promote is the stand-in)
		verdict.verdict = PlacementVerdict::Promote;
	}
	else if (m_graph.identityExposed[id])
	{
		verdict.verdict = PlacementVerdict::Stack;
	}
	else if (is_scalar_eligible(nodeType))
	{
		verdict.verdict = PlacementVerdict::Scalar;
	}
	else
	{
		verdict.verdict = PlacementVerdict::Stack;
	}

	return verdict;
}

void EscapeAnalyzer::assign_verdicts(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	for (const auto& pair : m_graph.siteIds)
	{
		StructSymbol* siteType = get_model().get_expression_type(pair.first);
		m_pragmatic.set_allocation_site_verdict(pair.first, classify_node(pair.second, siteType, summaries));
	}

	for (const auto& pair : m_graph.localIds)
	{
		int id = pair.second;

		StructSymbol* localType = nullptr;
		SyntaxKind localKind = pair.first->get_kind();
		if (localKind == SyntaxKind::VariableStatement)
			localType = static_cast<VariableSymbol*>(pair.first)->get_type();
		else if (localKind == SyntaxKind::Parameter)
			localType = static_cast<ParameterSymbol*>(pair.first)->get_type();

		m_pragmatic.set_local_verdict(m_graph.idOwners[id], pair.first, classify_node(id, localType, summaries));
	}

	commit_promotion_points();
}
