#include <shard/pragmatic/FrameLayout.hpp>

#include <gmt/Alignment.hpp>
#include <gmt/Containers.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

namespace
{
	std::size_t inline_cell_bytes(const TypeShape* shape)
	{
		// a stack-placed reference shape embeds a cell for every non-reference,
		// non-nullable field the value owns
		std::size_t bytes = 0;
		for (std::size_t i = 0; i < shape->get_slot_count(); ++i)
		{
			const TypeShape::SlotInfo& field = shape->get_slot(static_cast<std::uint32_t>(i));
			if (field.fieldShape == nullptr || field.fieldShape->is_reference() || field.isNullable)
				continue;

			bytes += gmt::align_up(field.fieldShape->get_size(), sizeof(void*));
		}

		return bytes;
	}

	std::size_t slot_data_bytes(const TypeShape* shape)
	{
		std::size_t bytes = gmt::align_up(shape->get_size(), sizeof(void*));
		if (!shape->is_reference())
			return bytes;

		return bytes + inline_cell_bytes(shape);
	}
}

FrameSlotRecipe FrameLayout::make_recipe(StructSymbol* type)
{
	FrameSlotRecipe recipe;

	// type parameters resolve through the instantiation's combined type
	// arguments (owner arguments first, then the method's own) when a
	// concrete frame is materialized for a generic method instantiation
	if (type != nullptr && type->get_kind() == SyntaxKind::TypeParameter)
	{
		recipe.typeParameterIndex = static_cast<std::int16_t>(static_cast<TypeParameterSymbol*>(type)->get_type_argument_index());
		return recipe;
	}

	recipe.concreteType = type;
	return recipe;
}

void FrameLayout::invalidate_slot_plans()
{
	m_slotPlansReady = false;
}

void FrameLayout::build_slot_plans() const
{
	const std::size_t localCount = static_cast<std::size_t>(m_argumentCount) + static_cast<std::size_t>(m_variableCount);
	m_slotPlans.assign(localCount, FrameSlotPlan{});
	m_slotPlansComplete = true;

	for (std::size_t i = 0; i < localCount; ++i)
	{
		FrameSlotPlan& plan = m_slotPlans[i];
		const bool isArgument = i < m_argumentCount;

		const TypeShape* shape = nullptr;
		bool isNullable = false;
		bool isOpen = false;
		PlacementVerdict verdict = PlacementVerdict::Heap;

		if (isArgument)
		{
			if (i < m_argumentShapes.size())
				shape = m_argumentShapes[i];

			StructSymbol* type = i < m_argumentTypes.size() ? m_argumentTypes[i] : nullptr;
			// a missing receiver type is open by convention: the value carries its shape
			isOpen = type == nullptr || type->get_kind() == SyntaxKind::TypeParameter;
			isNullable = type != nullptr && type->get_kind() == SyntaxKind::NullableType;
		}
		else
		{
			const std::size_t variableIndex = i - m_argumentCount;
			if (variableIndex < m_variableShapes.size())
				shape = m_variableShapes[variableIndex];

			if (variableIndex < m_variableSlots.size())
			{
				const FrameSlotRecipe& recipe = m_variableSlots[variableIndex];
				verdict = recipe.verdict;
				isOpen = recipe.typeParameterIndex >= 0
					|| (recipe.concreteType != nullptr && recipe.concreteType->get_kind() == SyntaxKind::TypeParameter);
				isNullable = recipe.concreteType != nullptr && recipe.concreteType->get_kind() == SyntaxKind::NullableType;
			}
		}

		FrameSlotFlag flags = FrameSlotFlag::None;
		if (isOpen)
			flags = flags | FrameSlotFlag::Open;

		if (isNullable)
			flags = flags | FrameSlotFlag::Nullable;

		if (shape != nullptr)
		{
			bool ownsData = !shape->is_reference();
			if (!isArgument)
			{
				ownsData = verdict == PlacementVerdict::Stack
					&& (!shape->is_reference() || shape->get_size() > 0);
			}

			if (ownsData)
			{
				const std::size_t slotBytes = slot_data_bytes(shape);
				if (slotBytes > 0xFFFFFFFF)
				{
					// the plan packs byte counts into 32 bits; a slot this large
					// goes through the size_t path instead
					m_slotPlansComplete = false;
				}
				else
				{
					flags = flags | FrameSlotFlag::OwnsData;
					plan.dataBytes = static_cast<std::uint32_t>(slotBytes);
				}
			}
			else if (!isArgument && !shape->is_reference() && !isNullable)
			{
				flags = flags | FrameSlotFlag::HeapCell;
			}
		}
		else if (!isOpen)
		{
			// a concrete slot without a resolved shape (a layout mutated through
			// the C API); the runtime falls back to resolving it once per call
			m_slotPlansComplete = false;
		}

		plan.shape = shape;
		plan.flags = flags;
	}

	m_slotPlansReady = true;
}

std::span<const FrameSlotPlan> FrameLayout::ensure_slot_plans() const
{
	if (!m_slotPlansReady)
		build_slot_plans();

	return std::span<const FrameSlotPlan>(m_slotPlans.data(), m_slotPlans.size());
}

bool FrameLayout::slot_plans_complete() const
{
	if (!m_slotPlansReady)
		build_slot_plans();

	return m_slotPlansComplete;
}

std::size_t FrameLayout::planned_locals_bytes() const
{
	if (!m_slotPlansReady)
		build_slot_plans();

	std::size_t bytes = 0;
	for (const FrameSlotPlan& plan : m_slotPlans)
		bytes += plan.dataBytes;

	return bytes;
}

std::uint16_t FrameLayout::add_variable(StructSymbol* type, PlacementVerdict verdict)
{
	FrameSlotRecipe recipe = make_recipe(type);
	recipe.verdict = verdict;

	m_variableSlots.push_back(std::move(recipe));
	invalidate_slot_plans();
	return m_variableCount++;
}

std::uint16_t FrameLayout::add_temporary(StructSymbol* type, const TypeShape* shape, PlacementVerdict verdict)
{
	std::uint16_t variableIndex = add_variable(type, verdict);

	// temporaries take their slot in the same order as declared locals, so the
	// per-slot shape table stays aligned with the variable indices
	m_variableShapes.push_back(shape);
	invalidate_slot_plans();

	m_localsBytes += entry_bytes(resolve_payload(shape, verdict));
	return variableIndex;
}

bool FrameLayout::seal_variable(std::uint16_t variableIndex, StructSymbol* type, PlacementVerdict verdict)
{
	if (variableIndex >= m_variableSlots.size())
		return false;

	FrameSlotRecipe& recipe = m_variableSlots[variableIndex];
	if (!(recipe.concreteType == nullptr) || recipe.typeParameterIndex >= 0)
		return false;

	recipe = make_recipe(type);
	recipe.verdict = verdict;
	invalidate_slot_plans();
	return true;
}

bool FrameLayout::set_variable_verdict(std::uint16_t variableIndex, PlacementVerdict verdict)
{
	if (variableIndex >= m_variableSlots.size())
		return false;

	m_variableSlots[variableIndex].verdict = verdict;
	invalidate_slot_plans();
	return true;
}

void FrameLayout::set_argument_count(std::uint16_t count)
{
	m_argumentCount = count;
	invalidate_slot_plans();
}

void FrameLayout::add_argument(StructSymbol* type)
{
	m_argumentTypes.push_back(type);
	invalidate_slot_plans();
}

const gmt::vector<StructSymbol*>& FrameLayout::get_argument_types() const
{
	return m_argumentTypes;
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

void FrameLayout::set_argument_shapes(gmt::vector<const TypeShape*> shapes)
{
	m_argumentShapes = std::move(shapes);
	invalidate_slot_plans();
}

void FrameLayout::set_variable_shapes(gmt::vector<const TypeShape*> shapes)
{
	m_variableShapes = std::move(shapes);
	invalidate_slot_plans();
}

const TypeShape* FrameLayout::get_argument_shape(std::size_t index) const
{
	return index < m_argumentShapes.size() ? m_argumentShapes[index] : nullptr;
}

const TypeShape* FrameLayout::get_variable_shape(std::size_t index) const
{
	return index < m_variableShapes.size() ? m_variableShapes[index] : nullptr;
}

std::size_t FrameLayout::get_argument_shape_count() const
{
	return m_argumentShapes.size();
}

std::size_t FrameLayout::get_variable_shape_count() const
{
	return m_variableShapes.size();
}

bool FrameLayout::has_return_shape() const
{
	return m_returnShapeResolved;
}

const TypeShape* FrameLayout::get_return_shape() const
{
	return m_returnShape;
}

void FrameLayout::set_return_shape(const TypeShape* shape) const
{
	m_returnShape = shape;
	m_returnShapeResolved = true;
}

const gmt::vector<FrameSlotRecipe>& FrameLayout::get_variable_slots() const
{
	return m_variableSlots;
}

void FrameLayout::set_locals_bytes(std::size_t bytes)
{
	m_localsBytes = bytes;
}

std::size_t FrameLayout::get_locals_bytes() const
{
	return m_localsBytes;
}

void FrameLayout::publish_eval_layout(std::uint32_t maxDepth, std::size_t maxPayload, bool poisoned)
{
	// the layout stops changing once its body is emitted, which is the point
	// where the per-slot plan can be resolved ahead of the first call
	build_slot_plans();

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
	if (shape == nullptr)
		return sizeof(void*);

	// primitives bypass the placement machinery: always inline storage
	if (!shape->is_reference())
		return shape->get_size();

	if (verdict == PlacementVerdict::Heap)
		return sizeof(void*);

	// variable-size heap objects (string, delegates, interfaces) carry no
	// inlinable payload even under a Stack verdict: pointer storage
	if (shape->get_size() == 0)
		return sizeof(void*);

	return shape->get_size();
}

std::size_t FrameLayout::entry_bytes(std::size_t payload)
{
	return EntryHeaderSize + gmt::align_up(payload, sizeof(void*));
}

std::size_t FrameLayout::compute_locals_bytes(std::span<const TypeShape*> argumentShapes, std::span<const TypeShape*> variableShapes) const
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
