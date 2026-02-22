#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <vector>

namespace shard
{
	/*
	template<typename T>
	class SHARD_API Vector
	{
		std::vector<T> myVector;

	public:
		Vector() = default;

		size_t Length();
		size_t Capacity();
		void Shrink();
		void Reserve(size_t newCapacity);

		bool IsEmpty();
		void Clear();

		T& ElementAt(size_t index);
		const T& ElementAt(size_t index) const;

		T& operator[](size_t index);
		const T& operator[](size_t index) const;

		T& First();
		const T& First() const;

		T& Last();
		const T& Last() const;

		T& Append(const T& value);
		T& Append(T&& value);
		T& Append(std::initializer_list<T> values);

		template<typename... Args>
		T& EmplaceAppend(Args&&... args);

		void Insert(size_t index, const T& value);
		void Insert(size_t index, T&& value);

		T RemoveFirst();
		T RemoveLast();

		T* RawData();
		const T* RawData() const;
	};
	*/
}
