#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/FuncCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

namespace shard
{
	// iterates a script IEnumerable<T> from native code with a range-for loop;
	// the adapter owns the enumerator and drives MoveNext/Current through the
	// virtual dispatch of the call state
	class SHARD_API EnumerableAdapter
	{
	public:
		class SHARD_API iterator
		{
			friend class EnumerableAdapter;

			FuncCallState* m_context = nullptr;
			ObjectInstance m_enumerator;
			ObjectInstance m_current;
			bool m_finished = true;

			iterator(FuncCallState& context, ObjectInstance enumerator);
			void advance();

		public:
			iterator() = default;

			ObjectInstance operator*() const;
			iterator& operator++();

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;
		};

	private:
		FuncCallState& m_context;
		ObjectInstance m_enumerable;

	public:
		EnumerableAdapter(FuncCallState& context, ObjectInstance enumerable);

		EnumerableAdapter(const EnumerableAdapter&) = delete;
		EnumerableAdapter& operator=(const EnumerableAdapter&) = delete;

		// the first element; end() when the sequence is empty or a call raised
		iterator begin();
		iterator end() const;
	};
}
