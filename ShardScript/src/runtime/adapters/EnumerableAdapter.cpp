#include <shard/runtime/adapters/EnumerableAdapter.hpp>

using namespace shard;

void Enumerable::iterator::invalid()
{
	m_context = nullptr;
	m_enumerator = ObjectInstance();
	m_current = ObjectInstance();
}

Enumerable::iterator::iterator(const CallState& context, ObjectInstance enumerator, ObjectInstance current)
	: m_context(&context), m_enumerator(enumerator), m_current(current)
{
	TypeSymbol* enumeratorType = const_cast<TypeSymbol*>(enumerator.getInfo());
	
	moveNext = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_MOVENEXT);
	if (moveNext == nullptr)
		throw undefined_behaviour(L"Enumerator does not implement MoveNext method");
	
	getCurrent = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_CURRENT_GET);
	if (getCurrent == nullptr)
		throw undefined_behaviour(L"Enumerator does not implement Current property accessor");
}

ObjectInstance Enumerable::iterator::operator*() const
{
	return m_current;
}

Enumerable::iterator& Enumerable::iterator::operator++()
{
	InvokeResult result = m_context->TryInvokeMethod(moveNext, { m_enumerator });
	if (!result.IsOk())
	{
		invalid();
		return *this;
	}

	ObjectInstance moved = result.Value();
	const bool hasNext = moved.AsBoolean();
	m_context->Collector.CollectInstance(moved);

	if (!hasNext)
	{
		invalid();
		return *this;
	}

	m_current = m_context->TryInvokeMethod(getCurrent, { m_enumerator }).Value();
	return *this;
}

bool Enumerable::iterator::operator==(const iterator& other) const
{
	if (m_context == nullptr && other.m_context == nullptr)
		return true;

	if (m_context == nullptr || other.m_context == nullptr)
		return false;

	return m_enumerator == other.m_enumerator;
}

bool Enumerable::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

Enumerable::Enumerable(const CallState& context, ObjectInstance enumerable)
	: m_context(context), m_enumerable(enumerable) { }

Enumerable::iterator Enumerable::begin()
{
	ObjectInstance enumerator = m_context.TryInvokeMethod(TRAIT_ENUMERABLE_GETENUMERATOR, { m_enumerable }).Value();
	if (enumerator.IsNullInstance())
		return end();

	iterator it = iterator(m_context, enumerator, ObjectInstance());
	++it;

	if (it.operator*().IsNullInstance())
		return end();

	return it;
}

Enumerable::iterator Enumerable::end()
{
	return iterator();
}
