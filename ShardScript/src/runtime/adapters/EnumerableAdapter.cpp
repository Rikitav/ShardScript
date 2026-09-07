#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/runtime/adapters/EnumerableAdapter.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

using namespace shard;

void EnumerableAdapter::iterator::invalid()
{
	m_context->Collector.CollectInstance(m_enumerator);

	m_context = nullptr;
	m_enumerator = ObjectInstance();
}

EnumerableAdapter::iterator::iterator(CallState* context, ObjectInstance enumerator)
	: m_context(context), m_enumerator(enumerator), m_current(nullptr, nullptr, nullptr)
{
	TypeSymbol* enumeratorType = const_cast<TypeSymbol*>(enumerator.getInfo());
	m_current = InvokeResult(m_context, nullptr, nullptr);

	moveNext = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_MOVENEXT);
	if (moveNext == nullptr)
		throw undefined_behaviour(L"Enumerator does not implement \"MoveNext\" method");

	getCurrent = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_CURRENT_GET);
	if (getCurrent == nullptr)
		throw undefined_behaviour(L"Enumerator does not implement \"Current\" property get accessor");
}

ObjectInstance EnumerableAdapter::iterator::operator*() const
{
	return m_current.Value();
}

EnumerableAdapter::iterator& EnumerableAdapter::iterator::operator++()
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

	m_current = m_context->TryInvokeMethod(getCurrent, { m_enumerator });
	if (!m_current)
	{
		invalid();
		return *this;
	}

	return *this;
}

bool EnumerableAdapter::iterator::operator==(const iterator& other) const
{
	if (m_context == nullptr && other.m_context == nullptr)
		return true;

	if (m_context == nullptr || other.m_context == nullptr)
		return false;

	return m_enumerator == other.m_enumerator;
}

bool EnumerableAdapter::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

EnumerableAdapter::EnumerableAdapter(const CallState& context, ObjectInstance EnumerableAdapter)
	: m_context(context), m_enumerable(EnumerableAdapter) {
}

EnumerableAdapter::iterator EnumerableAdapter::begin()
{
	ObjectInstance enumerator = m_context.TryInvokeMethod(TRAIT_ENUMERABLE_GETENUMERATOR, { m_enumerable }).Value();
	if (enumerator.IsNullInstance())
		return end();

	iterator it = iterator(const_cast<CallState*>(&m_context), enumerator);
	++it;

	if (it.operator*().IsNullInstance())
		return end();

	return it;
}

EnumerableAdapter::iterator EnumerableAdapter::end()
{
	return iterator(nullptr, null_instance);
}