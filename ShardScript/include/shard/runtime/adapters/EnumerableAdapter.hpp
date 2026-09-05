#pragma once
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

namespace shard
{
	class Enumerable
	{
		const CallState& m_context;
		ObjectInstance m_enumerable;

	public:
		class iterator
		{
			const CallState* m_context = nullptr;
			ObjectInstance m_enumerator;
			ObjectInstance m_current;

			MethodSymbol* moveNext = nullptr;
			MethodSymbol* getCurrent = nullptr;

			void invalid();

		public:
			iterator() = default;
			iterator(const CallState& context, ObjectInstance enumerator, ObjectInstance current);

			ObjectInstance operator*() const;
			iterator& operator++();

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;
		};

		Enumerable(const CallState& context, ObjectInstance enumerable);
		iterator begin();
		iterator end();
	};
}