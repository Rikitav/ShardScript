/*
#include <shard/runtime/EvalStack.hpp>

using namespace shard;

EvalStack::EvalStack(void* stackBase, std::size_t bytesSize) : StackBase(stackBase), BytesSize(bytesSize)
{

}

void EvalStack::Push(void* value, std::size_t size)
{
	if (BytesCursor + size > BytesSize)
		throw std::runtime_error("stack overflow");

	std::memcpy(value, StackBase[BytesCursor], size);
	BytesCursor += size;
}

void* EvalStack::Pop(std::size_t size)
{
	if (BytesCursor - size < 0)
		throw std::runtime_error("stack undeflow");

	BytesCursor -= size;
	void* value = StackBase[BytesCursor];
	return value;
}

void* EvalStack::Peek(std::size_t size)
{
	if (BytesCursor - size < 0)
		throw std::runtime_error("stack undeflow");

	return StackBase[BytesCursor - size];
}
*/
