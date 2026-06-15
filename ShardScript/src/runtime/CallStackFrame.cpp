#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

using namespace shard;

TypeSymbol* CallStackFrame::ResolveType(TypeSymbol* type)
{
	if (type == nullptr || type->Kind != SyntaxKind::TypeParameter)
		return type;

	TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(type);
	std::uint16_t index = typeParam->TypeArgumentIndex;
	if (index < TypeArguments.size())
		return TypeArguments[index];

	return type;
}

void CallStackFrame::PushStack(ObjectInstance* value)
{
	EvalStack.push_back(value);
}

ObjectInstance* CallStackFrame::PopStack()
{
	ObjectInstance* value = EvalStack.back();
	EvalStack.pop_back();
	return value;
}

ObjectInstance* CallStackFrame::PeekStack()
{
	return EvalStack.back();
}
