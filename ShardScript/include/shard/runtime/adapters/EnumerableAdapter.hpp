#pragma once
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

namespace shard
{
	class EnumerableAdapter
	{
		const CallState& m_context;
		ObjectInstance m_enumerable;

	public:
		class iterator
		{
			MethodSymbol* moveNext = nullptr;
			MethodSymbol* getCurrent = nullptr;

			CallState* m_context;
			ObjectInstance m_enumerator;
			InvokeResult m_current;

			void invalid();

		public:
			iterator() = default;
			iterator(CallState* context, ObjectInstance enumerator);

			ObjectInstance operator*() const;
			iterator& operator++();

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;
		};

		EnumerableAdapter(const CallState& context, ObjectInstance enumerable);
		iterator begin();
		iterator end();
	};
}