#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>

using namespace shard;

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
