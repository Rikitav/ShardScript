#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/GenericMethodSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

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

const gmt::unordered_map<gmt::Ref<const ExpressionSyntax>, AllocationSiteVerdict>& PragmaticModel::get_allocation_sites() const
{
	return m_allocationSites;
}

void PragmaticModel::set_promotion_points(gmt::Ref<const ExpressionSyntax> site, gmt::vector<gmt::Ref<const ExpressionSyntax>> points)
{
	m_promotionPoints[site] = std::move(points);
}

const gmt::vector<gmt::Ref<const ExpressionSyntax>>* PragmaticModel::get_promotion_points(gmt::Ref<const ExpressionSyntax> site) const
{
	auto it = m_promotionPoints.find(site);
	if (it == m_promotionPoints.end())
		return nullptr;

	return &it->second;
}

void PragmaticModel::set_local_verdict(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local, const AllocationSiteVerdict& verdict)
{
	m_methodLocals[method][local] = verdict;
}

AllocationSiteVerdict PragmaticModel::get_local_verdict(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local) const
{
	AllocationSiteVerdict fallback;
	fallback.verdict = PlacementVerdict::Heap; // conservative default for locals the escape pass did not see

	auto methodIt = m_methodLocals.find(method);
	if (methodIt == m_methodLocals.end())
		return fallback;

	auto localIt = methodIt->second.find(local);
	if (localIt == methodIt->second.end())
		return fallback;

	return localIt->second;
}

void PragmaticModel::set_local_order(gmt::Ref<MethodSymbol> method, gmt::vector<gmt::Ref<SyntaxSymbol>> locals)
{
	m_methodLocalOrder[method] = std::move(locals);
}

const gmt::vector<gmt::Ref<SyntaxSymbol>>* PragmaticModel::get_local_order(gmt::Ref<MethodSymbol> method) const
{
	auto it = m_methodLocalOrder.find(method);
	if (it == m_methodLocalOrder.end())
		return nullptr;

	return &it->second;
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

const gmt::unordered_map<gmt::Ref<MethodSymbol>, EscapeSummary>& PragmaticModel::get_summaries() const
{
	return m_summaries;
}

TypeShapeCache& PragmaticModel::get_type_shape_cache()
{
	return m_typeShapeCache;
}

TypeShape* PragmaticModel::find_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, std::span<const gmt::Ref<StructSymbol>> genericArgs) const
{
	return m_typeShapeCache.find_type_shape(model, baseType, genericArgs);
}

TypeShape* PragmaticModel::find_or_create_type_shape(SemanticModel& model, gmt::Ref<StructSymbol> baseType, std::span<const gmt::Ref<StructSymbol>> genericArgs)
{
	return m_typeShapeCache.find_or_create_type_shape(model, baseType, genericArgs);
}

FrameLayout* PragmaticModel::find_frame_layout(gmt::Ref<MethodSymbol> method) const
{
	auto choice = m_frameLayouts.find(method);
	if (choice == m_frameLayouts.end())
		return nullptr;

	return const_cast<FrameLayout*>(&choice->second);
}

FrameLayout& PragmaticModel::get_or_create_frame_layout(gmt::Ref<MethodSymbol> method)
{
	return m_frameLayouts[method];
}

FrameLayout& PragmaticModel::get_or_create_frame_layout(SemanticModel& model, gmt::Ref<MethodSymbol> method)
{
	auto choice = m_frameLayouts.find(method);
	if (choice != m_frameLayouts.end())
		return choice->second;

	if (!method->get_is_generic_instantiation())
		return m_frameLayouts[method];

	gmt::Ref<GenericMethodSymbol> instantiation = method.as<GenericMethodSymbol>();
	FrameLayout* open = find_frame_layout(instantiation->get_underlaying_method());
	if (open == nullptr)
		return m_frameLayouts[method];

	const gmt::vector<gmt::Ref<StructSymbol>>& arguments = instantiation->get_type_arguments();
	FrameLayout& layout = m_frameLayouts[method];

	layout.set_argument_count(open->get_argument_count());
	for (gmt::Ref<StructSymbol> argumentType : open->get_argument_types())
		layout.add_argument(model.substitute_type(argumentType, arguments));

	gmt::inline_vector<const TypeShape*, 8> argumentShapes;
	for (gmt::Ref<StructSymbol> argumentType : layout.get_argument_types())
		argumentShapes.push_back(argumentType.is_null() ? nullptr : find_or_create_type_shape(model, argumentType));

	gmt::inline_vector<const TypeShape*, 8> variableShapes;
	for (const FrameSlotRecipe& recipe : open->get_variable_slots())
	{
		gmt::Ref<StructSymbol> slotType = recipe.concreteType;
		if (recipe.typeParameterIndex >= 0)
		{
			const std::size_t index = static_cast<std::size_t>(recipe.typeParameterIndex);
			if (index < arguments.size())
				slotType = arguments[index];
		}
		else if (!slotType.is_null())
		{
			slotType = model.substitute_type(slotType, arguments);
		}

		layout.add_variable(slotType, recipe.verdict);
		variableShapes.push_back(slotType.is_null() ? nullptr : find_or_create_type_shape(model, slotType));
	}

	layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes.get_vector(), variableShapes.get_vector()));
	return layout;
}

void PragmaticModel::set_local_slot(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local, std::uint16_t slot)
{
	m_localSlots[method][local] = slot;
}

std::optional<std::uint16_t> PragmaticModel::get_local_slot(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> local) const
{
	auto methodChoice = m_localSlots.find(method);
	if (methodChoice == m_localSlots.end())
		return std::nullopt;

	auto slotChoice = methodChoice->second.find(local);
	if (slotChoice == methodChoice->second.end())
		return std::nullopt;

	return slotChoice->second;
}

gmt::vector<gmt::Ref<SyntaxSymbol>>& PragmaticModel::lambda_captures_for(gmt::Ref<MethodSymbol> method)
{
	return m_lambdaCaptures[method];
}

const gmt::vector<gmt::Ref<SyntaxSymbol>>* PragmaticModel::get_lambda_captures(gmt::Ref<MethodSymbol> method) const
{
	auto choice = m_lambdaCaptures.find(method);
	if (choice == m_lambdaCaptures.end())
		return nullptr;

	return &choice->second;
}
