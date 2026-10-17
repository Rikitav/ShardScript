#include <shard/pragmatic/FrameLayout.hpp>

#include <gmt/Alignment.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

FrameSlotRecipe FrameLayout::make_recipe(gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters)
{
	FrameSlotRecipe recipe;

	if (!type.is_null() && type->get_kind() == SyntaxKind::TypeParameter)
	{
		for (std::size_t i = 0; i < typeParameters.size(); ++i)
		{
			if (typeParameters[i] == type)
			{
				recipe.typeParameterIndex = static_cast<std::int16_t>(i);
				return recipe;
			}
		}
	}

	recipe.concreteType = type;
	return recipe;
}

std::uint16_t FrameLayout::add_variable(gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, PlacementVerdict verdict)
{
	FrameSlotRecipe recipe = make_recipe(type, typeParameters);
	recipe.verdict = verdict;

	m_variableSlots.push_back(std::move(recipe));
	return m_variableCount++;
}

bool FrameLayout::seal_variable(std::uint16_t variableIndex, gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, PlacementVerdict verdict)
{
	if (variableIndex >= m_variableSlots.size())
		return false;

	FrameSlotRecipe& recipe = m_variableSlots[variableIndex];
	if (!recipe.concreteType.is_null() || recipe.typeParameterIndex >= 0)
		return false;

	recipe = make_recipe(type, typeParameters);
	recipe.verdict = verdict;
	return true;
}

bool FrameLayout::set_variable_verdict(std::uint16_t variableIndex, PlacementVerdict verdict)
{
	if (variableIndex >= m_variableSlots.size())
		return false;

	m_variableSlots[variableIndex].verdict = verdict;
	return true;
}

void FrameLayout::set_argument_count(std::uint16_t count)
{
	m_argumentCount = count;
}

std::uint16_t FrameLayout::get_argument_count() const
{
	return m_argumentCount;
}

std::uint16_t FrameLayout::get_variable_count() const
{
	return m_variableCount;
}

std::uint16_t FrameLayout::get_local_count() const
{
	return m_argumentCount + m_variableCount;
}

const std::vector<FrameSlotRecipe>& FrameLayout::get_variable_slots() const
{
	return m_variableSlots;
}

void FrameLayout::publish_eval_layout(std::uint32_t maxDepth, std::size_t maxPayload, bool poisoned)
{
	// a poisoned tracker means the tracking desynced: publish nothing so the
	// runtime falls back to a conservatively sized frame instead of one that
	// could be under-sized
	if (poisoned)
		return;

	m_maxEvalDepth = maxDepth;
	m_evalSlotPayload = maxDepth > 0
		? (maxPayload > 1 ? maxPayload : 1) : 0;
	m_isComplete = true;
}

std::uint32_t FrameLayout::get_max_eval_depth() const
{
	return m_maxEvalDepth;
}

std::size_t FrameLayout::get_eval_slot_payload() const
{
	return m_evalSlotPayload;
}

bool FrameLayout::is_complete() const
{
	return m_isComplete;
}

std::size_t FrameLayout::resolve_payload(const TypeShape* shape, PlacementVerdict verdict)
{
	if (shape == nullptr || verdict == PlacementVerdict::Heap)
		return sizeof(void*);

	if (verdict == PlacementVerdict::Scalar)
		return 0; // dematerialized: the layout pass expands the fields into their own slots

	return shape->get_size();
}

std::size_t FrameLayout::entry_bytes(std::size_t payload)
{
	return EntryHeaderSize + gmt::align_up(payload, sizeof(void*));
}

std::size_t FrameLayout::compute_locals_bytes(const std::vector<const TypeShape*>& argumentShapes, const std::vector<const TypeShape*>& variableShapes) const
{
	std::size_t bytes = 0;

	// arguments stay caller-owned references, whatever the value's own
	// verdict: the caller's frame (or the heap) holds the instance
	for (const TypeShape* shape : argumentShapes)
		bytes += entry_bytes(resolve_payload(shape, PlacementVerdict::Heap));

	for (std::size_t i = 0; i < m_variableSlots.size() && i < variableShapes.size(); ++i)
	{
		const FrameSlotRecipe& recipe = m_variableSlots[i];
		if (recipe.typeParameterIndex >= 0)
			bytes += entry_bytes(sizeof(void*));
		else
			bytes += entry_bytes(resolve_payload(variableShapes[i], recipe.verdict));
	}

	return bytes;
}
