#include <shard/runtime/adapters/EnumerableAdapter.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/StandardTypes.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/VirtualMachine.hpp>

namespace
{
	shard::MethodSymbol* standard_method(shard::FuncCallState& context, shard::StandardTypes slot)
	{
		shard::SymbolTable& table = context.get_vm().get_model().get_table();
		shard::SyntaxSymbol* symbol = table.get_standard_type(slot);
		if ((symbol == nullptr) || !symbol->is_method())
			return nullptr;

		return static_cast<shard::MethodSymbol*>(symbol);
	}
}

using namespace shard;

EnumerableAdapter::EnumerableAdapter(FuncCallState& context, ObjectInstance enumerable)
	: m_context(context)
	, m_enumerable(enumerable)
{ }

EnumerableAdapter::iterator::iterator(FuncCallState& context, ObjectInstance enumerator)
	: m_context(&context)
	, m_enumerator(enumerator)
	, m_finished(false)
{ }

EnumerableAdapter::iterator EnumerableAdapter::begin()
{
	MethodSymbol* getEnumerator = standard_method(m_context, StandardTypes::IEnumerable_GetEnumerator);
	if ((getEnumerator == nullptr) || (m_enumerable == nullptr))
		return end();

	InvokeResult created = m_context.try_invoke_method(getEnumerator, { m_enumerable });
	if (!created.is_ok())
	{
		m_context.propagate(created.get_exception());
		return end();
	}

	iterator first(m_context, created.get_value());
	first.advance();
	return first;
}

EnumerableAdapter::iterator EnumerableAdapter::end() const
{
	return iterator();
}

void EnumerableAdapter::iterator::advance()
{
	if (m_finished || (m_context == nullptr))
		return;

	MethodSymbol* moveNext = standard_method(*m_context, StandardTypes::IEnumerator_MoveNext);
	if (moveNext == nullptr)
	{
		m_finished = true;
		return;
	}

	InvokeResult stepped = m_context->try_invoke_method(moveNext, { m_enumerator });
	if (!stepped.is_ok())
	{
		m_context->propagate(stepped.get_exception());
		m_finished = true;
		return;
	}

	if (!stepped.get_value().as_boolean())
	{
		m_finished = true;
		m_current = ObjectInstance::null();
		return;
	}

	MethodSymbol* currentGetter = standard_method(*m_context, StandardTypes::IEnumerator_Current_get);
	if (currentGetter == nullptr)
	{
		m_finished = true;
		return;
	}

	InvokeResult current = m_context->try_invoke_method(currentGetter, { m_enumerator });
	if (!current.is_ok())
	{
		m_context->propagate(current.get_exception());
		m_finished = true;
		return;
	}

	m_current = current.get_value();
}

ObjectInstance EnumerableAdapter::iterator::operator*() const
{
	return m_current;
}

EnumerableAdapter::iterator& EnumerableAdapter::iterator::operator++()
{
	advance();
	return *this;
}

bool EnumerableAdapter::iterator::operator==(const iterator& other) const
{
	if (m_finished || other.m_finished)
		return m_finished == other.m_finished;

	return m_enumerator.get_memory() == other.m_enumerator.get_memory();
}

bool EnumerableAdapter::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}
