#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/EscapeSummary.hpp>
#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/TypeShapeCache.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <optional>
#include <unordered_map>
#include <vector>

namespace shard
{
	class ExpressionSyntax;
	class MethodSymbol;
	class SemanticModel;
	class StructSymbol;

	class SHARD_API PragmaticModel
	{
		// FrameLayout is the intended consumer of the per-method local levels
		std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary> m_summaries;
		std::unordered_map<gmt::Ref<MethodSymbol>, std::unordered_map<gmt::Ref<SyntaxSymbol>, EscapeLevel>> m_methodLocals;
		std::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict> m_allocationSites;
		std::unordered_map<gmt::Ref<const ExpressionSyntax>, std::vector<gmt::Ref<const ExpressionSyntax>>> m_promotionPoints;

		// per-type memory shapes
		TypeShapeCache m_typeShapeCache;

		// per-method frame metadata
		std::unordered_map<gmt::Ref<MethodSymbol>, FrameLayout> m_frameLayouts;

	public:
		PragmaticModel() = default;
		~PragmaticModel() = default;

		PragmaticModel(const PragmaticModel&) = delete;
		PragmaticModel& operator=(const PragmaticModel&) = delete;

		void set_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site, const AllocationSiteVerdict& verdict);
		std::optional<AllocationSiteVerdict> get_allocation_site_verdict(gmt::Ref<const ExpressionSyntax> site) const;

		const std::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict>& get_allocation_sites() const;

		void set_promotion_points(gmt::Ref<const ExpressionSyntax> site, std::vector<gmt::Ref<const ExpressionSyntax>> points);
		// nullptr when the site is not promoted: no migration points recorded
		const std::vector<gmt::Ref<const ExpressionSyntax>>* get_promotion_points(gmt::Ref<const ExpressionSyntax> site) const;

		void set_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local, EscapeLevel level);
		EscapeLevel get_local_level(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local) const;

		void set_summary(gmt::Ref<MethodSymbol> method, const EscapeSummary& summary);
		EscapeSummary get_summary(gmt::Ref<MethodSymbol> method) const;

		const std::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& get_summaries() const;

		TypeShapeCache& get_type_shape_cache();

		TypeShape* find_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, const std::vector<gmt::Ref<StructSymbol>>& genericArgs = {}) const;
		TypeShape* find_or_create_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, const std::vector<gmt::Ref<StructSymbol>>& genericArgs = {});

		FrameLayout* find_frame_layout(gmt::Ref<MethodSymbol> method) const;
		FrameLayout& get_or_create_frame_layout(gmt::Ref<MethodSymbol> method);
	};
}
