#include <shard/runtime/EvalStack.h>

/*
using namespace shard;

EvalStack::EvalStack(void* stackBase, size_t bytesSize) : StackBase(stackBase), BytesSize(bytesSize)
{

}

void EvalStack::Push(void* value, size_t size)
{
	if (BytesCursor + size > BytesSize)
		throw std::runtime_error("stack overflow");

	memcpy(value, StackBase[BytesCursor], size);
	BytesCursor += size;
}

void* EvalStack::Pop(size_t size)
{
	if (BytesCursor - size < 0)
		throw std::runtime_error("stack undeflow");

	BytesCursor -= size;
	void* value = StackBase[BytesCursor];
	return value;
}

void* EvalStack::Peek(size_t size)
{
	if (BytesCursor - size < 0)
		throw std::runtime_error("stack undeflow");

	return StackBase[BytesCursor - size];
}
*/
