#pragma once
#include <shard/Definitions.hpp>

#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/EscapeSummary.hpp>
#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/TypeShapeCache.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <optional>
#include <unordered_map>
#include <span>
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
		gmt::unordered_map<MethodSymbol*, EscapeSummary> m_summaries;
		gmt::unordered_map<MethodSymbol*, gmt::unordered_map<SyntaxSymbol*, AllocationSiteVerdict>> m_methodLocals;
		gmt::unordered_map<MethodSymbol*, gmt::vector<SyntaxSymbol*>> m_methodLocalOrder; // declaration order, for frame slot indexing
		gmt::unordered_map<const ExpressionSyntax*, AllocationSiteVerdict> m_allocationSites;
		gmt::unordered_map<const ExpressionSyntax*, gmt::vector<const ExpressionSyntax*>> m_promotionPoints;

		// per-type memory shapes
		TypeShapeCache m_typeShapeCache;

		// per-method frame metadata
		gmt::unordered_map<MethodSymbol*, FrameLayout> m_frameLayouts;

		// absolute frame slot per local symbol (argument slots included in the numbering)
		gmt::unordered_map<MethodSymbol*, gmt::unordered_map<SyntaxSymbol*, std::uint16_t>> m_localSlots;

		// ordered symbols captured by each lambda method, matching its capture
		// box field order; empty for non-capturing lambdas
		gmt::unordered_map<MethodSymbol*, gmt::vector<SyntaxSymbol*>> m_lambdaCaptures;

	public:
		PragmaticModel() = default;
		~PragmaticModel() = default;

		PragmaticModel(const PragmaticModel&) = delete;
		PragmaticModel& operator=(const PragmaticModel&) = delete;

		void set_allocation_site_verdict(const ExpressionSyntax* site, const AllocationSiteVerdict& verdict);
		std::optional<AllocationSiteVerdict> get_allocation_site_verdict(const ExpressionSyntax* site) const;

		const gmt::unordered_map<const ExpressionSyntax*, AllocationSiteVerdict>& get_allocation_sites() const;

		void set_promotion_points(const ExpressionSyntax* site, gmt::vector<const ExpressionSyntax*> points);
		// nullptr when the site is not promoted: no migration points recorded
		const gmt::vector<const ExpressionSyntax*>* get_promotion_points(const ExpressionSyntax* site) const;

		void set_local_verdict(MethodSymbol* method, SyntaxSymbol* local, const AllocationSiteVerdict& verdict);
		AllocationSiteVerdict get_local_verdict(MethodSymbol* method, SyntaxSymbol* local) const;

		void set_local_order(MethodSymbol* method, gmt::vector<SyntaxSymbol*> locals);
		const gmt::vector<SyntaxSymbol*>* get_local_order(MethodSymbol* method) const;

		void set_summary(MethodSymbol* method, const EscapeSummary& summary);
		EscapeSummary get_summary(MethodSymbol* method) const;

		const gmt::unordered_map<MethodSymbol*, EscapeSummary>& get_summaries() const;

		TypeShapeCache& get_type_shape_cache();

		TypeShape* find_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs = {}) const;
		TypeShape* find_or_create_type_shape(SemanticModel& model, StructSymbol* baseType, std::span<StructSymbol* const> genericArgs = {});

		FrameLayout* find_frame_layout(MethodSymbol* method) const;
		FrameLayout& get_or_create_frame_layout(MethodSymbol* method);
		FrameLayout& get_or_create_frame_layout(SemanticModel& model, MethodSymbol* method);

		void set_local_slot(MethodSymbol* method, SyntaxSymbol* local, std::uint16_t slot);
		std::optional<std::uint16_t> get_local_slot(MethodSymbol* method, SyntaxSymbol* local) const;

		gmt::vector<SyntaxSymbol*>& lambda_captures_for(MethodSymbol* method);
		const gmt::vector<SyntaxSymbol*>* get_lambda_captures(MethodSymbol* method) const;
	};
}
