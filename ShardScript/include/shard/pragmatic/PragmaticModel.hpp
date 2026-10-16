#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/PlacementVerdict.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <optional>
#include <unordered_map>

namespace shard
{
	class ExpressionSyntax;
	class MethodSymbol;

	class SHARD_API PragmaticModel
	{
		// FrameLayout is the intended consumer of the per-method local levels
		std::unordered_map<gmt::Ref<MethodSymbol>, std::unordered_map<gmt::Ref<SyntaxSymbol>, EscapeLevel>> m_methodLocals;
		std::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict> m_allocationSites;

	public:
		PragmaticModel() = default;
		~PragmaticModel() = default;

		PragmaticModel(const PragmaticModel&) = delete;
		PragmaticModel& operator=(const PragmaticModel&) = delete;

		void set_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site, const AllocationSiteVerdict& verdict);
		std::optional<AllocationSiteVerdict> get_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site) const;

		const std::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict>& get_allocation_sites() const;

		void set_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local, EscapeLevel level);
		EscapeLevel get_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local) const;
	};
}
