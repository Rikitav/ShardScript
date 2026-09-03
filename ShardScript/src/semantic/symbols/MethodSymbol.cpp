#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/TypeLayout.hpp>

using namespace shard;

namespace
{
	static FrameSlotRecipe MakeSlotRecipe(TypeSymbol* type, const std::vector<TypeParameterSymbol*>& typeParameters)
	{
		FrameSlotRecipe recipe;
		if (type != nullptr && type->Kind == SyntaxKind::TypeParameter)
		{
			for (std::size_t i = 0; i < typeParameters.size(); ++i)
			{
				if (typeParameters[i] == type)
				{
					recipe.TypeParameterIndex = static_cast<std::int16_t>(i);
					return recipe;
				}
			}
		}

		recipe.ConcreteType = type;
		return recipe;
	}

	static std::size_t EntryBytes(std::size_t payload)
	{
		return FrameLayout::EntryHeaderSize + AlignUp(payload, sizeof(void*));
	}
}

bool MethodSymbol::IsMethod() const
{
	return true;
}

void MethodSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::TypeParameter:
		{
			TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(symbol);
			typeParam->Parent = this;
			TypeParameters.push_back(typeParam);
			break;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* param = static_cast<ParameterSymbol*>(symbol);
			param->Parent = this;
			Parameters.push_back(param);
			break;
		}
	}
}

std::uint16_t MethodSymbol::GetEvalStackArgumentsCount() const
{
    std::uint16_t count = static_cast<std::uint16_t>(Parameters.size());
    if (Linking == LINK_INSTANCE)
        count += 1; // implicit 'this'

    return count;
}

std::uint16_t MethodSymbol::GetEvalStackVariablesCount() const
{
    return EvalStackVariablesCount;
}

std::uint16_t MethodSymbol::GetEvalStackLocalsCount() const
{
    return GetEvalStackArgumentsCount() + EvalStackVariablesCount;
}

std::uint16_t MethodSymbol::AddVariableCount(TypeSymbol* type)
{
	Layout.VariableSlots.push_back(MakeSlotRecipe(type, TypeParameters));
	return EvalStackVariablesCount++;
}

void MethodSymbol::SealVariableSlot(std::uint16_t slotIndex, TypeSymbol* type)
{
	std::uint16_t argsCount = GetEvalStackArgumentsCount();
	if (slotIndex < argsCount || slotIndex - argsCount >= Layout.VariableSlots.size())
		return;

	FrameSlotRecipe& recipe = Layout.VariableSlots[slotIndex - argsCount];
	if (recipe.ConcreteType != nullptr || recipe.TypeParameterIndex >= 0)
		return;

	recipe = MakeSlotRecipe(type, TypeParameters);
}

std::size_t FrameLayout::ResolveTypePayload(TypeSymbol* type)
{
	if (type == nullptr)
		return sizeof(void*); // unknown: reference-sized; Stage 4 boxes by-value overflows

	if (type->IsReferenceType())
		return sizeof(void*);

	if (type->Kind == SyntaxKind::TypeParameter)
		return sizeof(void*); // per-invocation resolution through TypeArguments arrives with Stage 4

	return type->GetInlineSize();
}

std::size_t FrameLayout::ComputeLocalsBytes(const MethodSymbol& method) const
{
	std::size_t bytes = 0;

	if (method.Linking == LINK_INSTANCE)
		bytes += EntryBytes(sizeof(void*)); // implicit 'this'

	for (const ParameterSymbol* parameter : method.Parameters)
		bytes += EntryBytes(ResolveTypePayload(parameter != nullptr ? const_cast<TypeSymbol*>(parameter->Type) : nullptr));

	for (const FrameSlotRecipe& recipe : VariableSlots)
	{
		if (recipe.TypeParameterIndex >= 0)
			bytes += EntryBytes(sizeof(void*)); // unresolved generic parameter: reference-sized
		else
			bytes += EntryBytes(ResolveTypePayload(recipe.ConcreteType));
	}

	return bytes;
}
