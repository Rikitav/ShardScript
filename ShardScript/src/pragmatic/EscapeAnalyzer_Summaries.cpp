#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <stdexcept>

using namespace shard;

void EscapeAnalyzer::analyze(SyntaxTree& tree)
{
	// deliberate simple design: the whole graph is rebuilt on every outer
	// iteration; every update is a monotone join, so the loop terminates
	// once summaries and levels stabilize
	for (int iteration = 0; iteration < 64; ++iteration)
	{
		clear_graph();
		visit_syntax_tree(tree);
		run_graph_fixpoint();

		std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary> summaries = derive_summaries();
		apply_call_exposures(summaries);
		run_graph_fixpoint();

		bool changed = m_previousLevels != m_graph.levels;
		changed = changed || m_previousChannels != m_graph.channelSets;
		changed = changed || m_previousColdChannels != m_graph.coldChannels;
		changed = changed || commit_summaries(summaries);

		m_previousLevels = m_graph.levels;
		m_previousChannels = m_graph.channelSets;
		m_previousColdChannels = m_graph.coldChannels;

		assign_verdicts(summaries);

		if (!changed)
			return;
	}

	throw std::runtime_error("pragmatic escape analysis did not reach a stable fixpoint");
}

ParameterEscapeSummary EscapeAnalyzer::derive_parameter_summary(gmt::Ref<MethodSymbol> method, gmt::Ref<ParameterSymbol> parameter, bool isReceiver)
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

std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary> EscapeAnalyzer::derive_summaries()
{
	std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary> result;

	for (const WalkedMethod& walked : m_walked)
	{
		gmt::Ref<MethodSymbol> method = walked.method;
		if (method.is_null() || result.find(method) != result.end())
			continue;

		EscapeSummary summary;
		summary.computed = true;
		summary.returnType = method->get_return_type();

		if (method->get_linking() == LINK_INSTANCE)
			summary.parameters.push_back(derive_parameter_summary(method, gmt::Ref<ParameterSymbol>(), true));

		for (gmt::Ref<ParameterSymbol> parameter : method->get_parameters())
			summary.parameters.push_back(derive_parameter_summary(method, parameter, false));

		result[method] = summary;
	}

	for (const WalkedMethod& walked : m_walked)
	{
		if (walked.method.is_null())
			continue;

		result[walked.method].returnDisposition = classify_returns(walked, result);
	}

	return result;
}

void EscapeAnalyzer::apply_call_exposures(const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries)
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

bool EscapeAnalyzer::commit_summaries(const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries)
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

int EscapeAnalyzer::classify_local_roots(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local)
{
	auto startIt = m_graph.localIds.find(local);
	if (startIt == m_graph.localIds.end())
		return 0;

	const EscapeChannel taint = EscapeChannel::Store | EscapeChannel::Capture | EscapeChannel::Suspension | EscapeChannel::Exception;

	int mark = 0;
	std::unordered_set<int> visited;
	std::vector<int> stack = { startIt->second };

	while (!stack.empty())
	{
		int id = stack.back();
		stack.pop_back();

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

int EscapeAnalyzer::classify_return_expression(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> expression, const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries)
{
	ExpressionRefs refs;
	collect_expression_refs(expression, refs, false);

	int mark = 0;

	for (gmt::Ref<const ExpressionSyntax> site : refs.sites)
	{
		auto it = m_graph.siteIds.find(site);
		if (it == m_graph.siteIds.end())
			return 0;

		if (!(m_graph.idOwners[it->second] == method))
			return 0;

		mark |= 1;
	}

	for (gmt::Ref<SyntaxSymbol> local : refs.locals)
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
		gmt::Ref<MethodSymbol> callee;
		auto nodeRef = ref_from(expression.as_ptr());
		if (nodeRef.has_value())
		{
			gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
			if (!bound.is_null() && bound->is_method())
				callee = bound.as<MethodSymbol>();
		}

		if (!callee.is_null())
		{
			auto it = summaries.find(callee);
			if (it != summaries.end() && it->second.returnDisposition == ReturnDisposition::Fresh)
				mark = 1;
		}
	}

	return mark;
}

ReturnDisposition EscapeAnalyzer::classify_returns(const WalkedMethod& walked, const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries)
{
	if (walked.returnedExpressions.empty())
		return ReturnDisposition::Unknown;

	int combinedMark = 0;
	int contributions = 0;

	for (gmt::Ref<const ExpressionSyntax> expression : walked.returnedExpressions)
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

bool EscapeAnalyzer::is_caller_placeable(gmt::Ref<MethodSymbol> owner) const
{
	if (owner.is_null())
		return false;

	gmt::Ref<StructSymbol> returnType = owner->get_return_type();
	if (returnType.is_null())
		return false;

	// concrete types only: interfaces (unknown size + vtable identity),
	// nullables, generics, arrays and type parameters stay heap
	return returnType->get_kind() == SyntaxKind::StructDeclaration;
}

bool EscapeAnalyzer::cone_closed(int id, const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries) const
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

bool EscapeAnalyzer::participates_in_reference_cycle(gmt::Ref<StructSymbol> type)
{
	std::unordered_set<gmt::Ref<StructSymbol>> visited;
	std::vector<gmt::Ref<StructSymbol>> stack = { type };

	while (!stack.empty())
	{
		gmt::Ref<StructSymbol> current = stack.back();
		stack.pop_back();

		for (gmt::Ref<FieldSymbol> field : current->fields)
		{
			gmt::Ref<StructSymbol> fieldType = field->get_return_type();
			if (fieldType.is_null() || fieldType->get_kind() != SyntaxKind::StructDeclaration)
				continue;

			if (fieldType == type)
				return true;

			if (visited.insert(fieldType).second)
				stack.push_back(fieldType);
		}
	}

	return false;
}

bool EscapeAnalyzer::is_scalar_eligible(gmt::Ref<StructSymbol> type)
{
	if (type.is_null())
		return false;

	if (type->get_kind() != SyntaxKind::StructDeclaration)
		return false;

	auto cached = m_scalarCache.find(type);
	if (cached != m_scalarCache.end())
		return cached->second;

	bool eligible = true;

	for (gmt::Ref<FieldSymbol> field : type->fields)
	{
		gmt::Ref<StructSymbol> fieldType = field->get_return_type();
		bool nullableField = !fieldType.is_null();
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

void EscapeAnalyzer::assign_verdicts(const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& summaries)
{
	for (const auto& pair : m_graph.siteIds)
	{
		int id = pair.second;

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
		else if (is_scalar_eligible(get_model().get_expression_type(pair.first)))
		{
			verdict.verdict = PlacementVerdict::Scalar;
		}
		else
		{
			verdict.verdict = PlacementVerdict::Stack;
		}

		m_pragmatic.set_allocation_site_verdict(pair.first, verdict);
	}

	for (const auto& pair : m_graph.localIds)
		m_pragmatic.set_local_level(m_graph.idOwners[pair.second], pair.first, m_graph.levels[pair.second]);

	commit_promotion_points();
}
