#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

#include <gmt/InlineVector.hpp>

#include <unordered_set>
#include <utility>
#include <vector>

using namespace shard;

void EscapeAnalyzer::record_cold_point(int id, gmt::Ref<const ExpressionSyntax> expression)
{
	m_coldPoints.emplace_back(id, expression);
}

bool EscapeAnalyzer::reaches_site(int from, int to) const
{
	std::unordered_set<int> visited;
	gmt::inline_vector<int, 16> stack;
	stack.push_back(from);

	while (!stack.get_vector().empty())
	{
		int id = stack.get_vector().back();
		stack.get_vector().pop_back();

		if (id == to)
			return true;

		if (!visited.insert(id).second)
			continue;

		for (const std::pair<int, int>& edge : m_graph.edges)
		{
			if (edge.first == id)
				stack.push_back(edge.second);
		}
	}

	return false;
}

void EscapeAnalyzer::commit_promotion_points()
{
	for (const auto& pair : m_graph.siteIds)
	{
		std::optional<AllocationSiteVerdict> verdict = m_pragmatic.get_allocation_site_verdict(pair.first);
		if (!verdict.has_value() || verdict->verdict != PlacementVerdict::Promote)
			continue;

		std::vector<gmt::Ref<const ExpressionSyntax>> points;
		for (const std::pair<int, gmt::Ref<const ExpressionSyntax>>& point : m_coldPoints)
		{
			// a cold exposure is recorded against the local it names; the
			// site it aliases (possibly transitively) inherits the point
			if (!reaches_site(point.first, pair.second))
				continue;

			bool duplicate = false;
			for (gmt::Ref<const ExpressionSyntax> existing : points)
			{
				if (existing == point.second)
				{
					duplicate = true;
					break;
				}
			}

			if (!duplicate)
				points.push_back(point.second);
		}

		// alias-member fixups beyond the recorded expressions are not stored
		// here: lowering derives them from the constraint graph's alias edges
		// when migration lists exist
		m_pragmatic.set_promotion_points(pair.first, std::move(points));
	}
}
