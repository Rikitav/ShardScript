#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include "LocalType.hpp"

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

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
		auto receiver = m_graph.receiverIds.find(method);
		if (receiver != m_graph.receiverIds.end())
			id = receiver->second;
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
		if (walked.method == nullptr)
			continue;

		result[walked.method].returnDisposition = classify_returns(walked, result);
	}

	return result;
}

void EscapeAnalyzer::apply_call_exposures(const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	for (const Graph::CallExposure& exposure : m_graph.callExposures)
	{
		EscapeSummary precomputed;
		const EscapeSummary* summaryPtr = nullptr;

		auto it = summaries.find(exposure.callee);
		if (it != summaries.end() && it->second.computed)
		{
			summaryPtr = &it->second;
		}
		else
		{
			precomputed = m_pragmatic.get_summary(exposure.callee);
			if (precomputed.computed)
				summaryPtr = &precomputed;
		}

		// a callee with no derived summary was never walked in this tree
		// (libfunc, bodyless member): the trusted conservative default
		if (summaryPtr == nullptr)
		{
			add_event(exposure.nodeId, EscapeLevel::GlobalEscape, EscapeChannel::UnknownCallee, exposure.cold);
			continue;
		}

		const EscapeSummary& summary = *summaryPtr;
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

		if (m_graph.allocationNodes[id])
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

		for (int successor : m_graph.successors[id])
			stack.push_back(successor);
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
		if (expression != nullptr)
		{
			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(expression);
			if (bound != nullptr && bound->is_method())
				callee = static_cast<MethodSymbol*>(bound);
		}

		if (callee != nullptr)
		{
			auto it = summaries.find(callee);
			if (it != summaries.end() && it->second.returnDisposition == ReturnDisposition::Fresh)
			{
				mark = 1;
			}
			else
			{
				EscapeSummary precomputed = m_pragmatic.get_summary(callee);
				if (precomputed.computed && precomputed.returnDisposition == ReturnDisposition::Fresh)
					mark = 1;
			}
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

bool EscapeAnalyzer::cone_closed(int id, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries) const
{
	for (const Graph::CallExposure& exposure : m_graph.callExposures)
	{
		if (exposure.nodeId != id)
			continue;

		auto it = summaries.find(exposure.callee);
		if (it != summaries.end() && it->second.computed)
			continue;

		EscapeSummary precomputed = m_pragmatic.get_summary(exposure.callee);
		if (!precomputed.computed)
			return false;
	}

	return true;
}

AllocationSiteVerdict EscapeAnalyzer::classify_node(int id, StructSymbol* nodeType, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	AllocationSiteVerdict verdict;
	verdict.level = m_graph.levels[id];
	verdict.channels = m_graph.channelSets[id];

	if (m_graph.borrowedAlias[id] && get_model().is_reference_shaped(nodeType))
	{
		verdict.verdict = PlacementVerdict::Heap;
		return verdict;
	}

	if (verdict.level == EscapeLevel::GlobalEscape || m_graph.coldChannels[id] != EscapeChannel::None)
	{
		verdict.verdict = PlacementVerdict::Heap;
	}
	else if (verdict.level == EscapeLevel::ArgEscape)
	{
		verdict.verdict = cone_closed(id, summaries) ? PlacementVerdict::Stack : PlacementVerdict::Heap;
	}
	else
	{
		verdict.verdict = PlacementVerdict::Stack;
	}

	return verdict;
}

AllocationSiteVerdict EscapeAnalyzer::classify_local(int id, SyntaxSymbol* local, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)
{
	StructSymbol* type = local_type(local);
	AllocationSiteVerdict verdict = classify_node(id, type, summaries);
	if (local->get_kind() != SyntaxKind::Parameter && type != nullptr && type->get_kind() == SyntaxKind::ArrayType)
		verdict.verdict = PlacementVerdict::Heap;

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
		SyntaxSymbol* local = pair.first;
		int id = pair.second;
		MethodSymbol* owner = m_graph.idOwners[id];
		if (owner == nullptr)
			continue;

		m_pragmatic.set_local_verdict(owner, local, classify_local(id, local, summaries));
	}
}
