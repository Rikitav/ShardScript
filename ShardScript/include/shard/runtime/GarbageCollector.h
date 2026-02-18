#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <unordered_map>
#include <iterator>
#include <cstdint>

namespace shard
{
    class VirtualMachine;

    template<typename MapType>
    class SHARD_API ValueIterator
    {
    private:
        using MapIterator = typename MapType::iterator;
        MapIterator it;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename MapType::mapped_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        ValueIterator(MapIterator iterator) : it(iterator) {}

        value_type& operator*() { return it->second; }
        value_type* operator->() { return &it->second; }

        const value_type& operator*() const { return it->second; }
        const value_type* operator->() const { return &it->second; }

        ValueIterator& operator++() { ++it; return *this; }
        ValueIterator operator++(int) { ValueIterator temp = *this; ++it; return temp; }

        bool operator==(const ValueIterator& other) const { return it == other.it; }
        bool operator!=(const ValueIterator& other) const { return it != other.it; }
    };

	class SHARD_API InstancesHeap
	{
    private:
        std::unordered_map<void*, shard::ObjectInstance*> PtrMap;

    public:
        using iterator = ValueIterator<decltype(PtrMap)>;
        using const_iterator = ValueIterator<const decltype(PtrMap)>;

        inline iterator begin() { return iterator(PtrMap.begin()); }
        inline iterator end() { return iterator(PtrMap.end()); }

        inline auto pairs_begin() { return PtrMap.begin(); }
        inline auto pairs_end() { return PtrMap.end(); }

        inline shard::ObjectInstance* at(void* ptr) { return PtrMap.at(ptr); }

        inline void add(ObjectInstance* instance)
        {
            PtrMap[instance->Memory] = instance;
        }

        inline void erase(shard::ObjectInstance* instance)
        {
            PtrMap.erase(instance->Memory);
        }

        inline void clear()
        {
            PtrMap.clear();
        }

        inline size_t size()
        {
            return PtrMap.size();
        }
	};

	class SHARD_API GarbageCollector
	{
		inline static uint64_t objectsCounter = 0;
        inline static std::unordered_map<FieldSymbol*, ObjectInstance*> staticFields;
        
    public:
		inline static InstancesHeap Heap;
        static ObjectInstance* NullInstance;

        static ObjectInstance* GetStaticField(const VirtualMachine* host, FieldSymbol* field);
        static void SetStaticField(const VirtualMachine* host, FieldSymbol* field, ObjectInstance* instance);

		static ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo);
        static ObjectInstance* CopyInstance(ObjectInstance* instance);
		
        static void CollectInstance(ObjectInstance* instance);
        static void DestroyInstance(ObjectInstance* instance);
        static void TerminateInstance(ObjectInstance* instance);
		static void Terminate();
	};
}
