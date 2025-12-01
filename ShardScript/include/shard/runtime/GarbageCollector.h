#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <unordered_map>
#include <iterator>

namespace shard::runtime
{
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
        std::unordered_map<long, shard::runtime::ObjectInstance*> IdMap;
        std::unordered_map<void*, shard::runtime::ObjectInstance*> PtrMap;

    public:
        using iterator = ValueIterator<decltype(IdMap)>;
        using const_iterator = ValueIterator<const decltype(IdMap)>;

        inline void add(ObjectInstance* instance)
        {
            IdMap[instance->Id] = instance;
            PtrMap[(void*)instance->Ptr] = instance;
        }

        inline iterator begin() { return iterator(IdMap.begin()); }
        inline iterator end() { return iterator(IdMap.end()); }

        inline auto pairs_begin() { return IdMap.begin(); }
        inline auto pairs_end() { return IdMap.end(); }

        inline shard::runtime::ObjectInstance* at(long id) { return IdMap.at(id); }
        inline shard::runtime::ObjectInstance* at(void* ptr) { return PtrMap.at(ptr); }

        inline void erase(shard::runtime::ObjectInstance* instance)
        {
            IdMap.erase(instance->Id);
            PtrMap.erase((void*)instance->Ptr);
        }

        inline void clear()
        {
            IdMap.clear();
            PtrMap.clear();
        }

        inline size_t size()
        {
            return IdMap.size();
        }
	};

	class SHARD_API GarbageCollector
	{
		inline static long objectsCounter = 0;
        inline static std::unordered_map<shard::syntax::symbols::TypeSymbol*, ObjectInstance*> nullInstancesMap;
        inline static std::unordered_map<shard::syntax::symbols::FieldSymbol*, ObjectInstance*> staticFields;
        
    public:
		inline static InstancesHeap Heap;
        static ObjectInstance* NullInstance;

        static ObjectInstance* GetStaticField(shard::syntax::symbols::FieldSymbol* field);
        static void SetStaticField(shard::syntax::symbols::FieldSymbol* field, ObjectInstance* instance);

		static ObjectInstance* AllocateInstance(const shard::syntax::symbols::TypeSymbol* objectInfo);
        static ObjectInstance* CopyInstance(const shard::syntax::symbols::TypeSymbol* objectInfo, void* ptr);
        static ObjectInstance* CopyInstance(ObjectInstance* instance);
		
        static void CollectInstance(ObjectInstance* instance);
        static void DestroyInstance(ObjectInstance* instance);
        static void TerminateInstance(ObjectInstance* instance);
		static void Terminate();
	};
}
