#include <shard/pragmatic/PragmaticModel.hpp>

using namespace shard;

void PragmaticModel::set_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site, const AllocationSiteVerdict& verdict)
{
	m_allocationSites[site] = verdict;
}

std::optional<AllocationSiteVerdict> PragmaticModel::get_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site) const
{
	auto it = m_allocationSites.find(site);
	if (it == m_allocationSites.end())
		return std::nullopt;

	return it->second;
}

const std::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict>& PragmaticModel::get_allocation_sites() const
{
	return m_allocationSites;
}

void PragmaticModel::set_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local, EscapeLevel level)
{
	m_methodLocals[method][local] = level;
}

EscapeLevel PragmaticModel::get_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local) const
{
	auto methodIt = m_methodLocals.find(method);
	if (methodIt == m_methodLocals.end())
		return EscapeLevel::NoEscape;

	auto localIt = methodIt->second.find(local);
	if (localIt == methodIt->second.end())
		return EscapeLevel::NoEscape;

	return localIt->second;
}

void PragmaticModel::set_summary(gmt::Ref<MethodSymbol> method, const EscapeSummary& summary)
{
	m_summaries[method] = summary;
}

EscapeSummary PragmaticModel::get_summary(gmt::Ref<MethodSymbol> method) const
{
	auto it = m_summaries.find(method);
	if (it == m_summaries.end())
		return EscapeSummary();

	return it->second;
}

const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& PragmaticModel::get_summaries() const
{
	return m_summaries;
}
