#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>

#include <unordered_map>
#include <iterator>
#include <cstdint>

namespace shard
{
    class ApplicationDomain;

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
        std::unordered_map<void*, ObjectInstance*> PtrMap;

    public:
        using iterator = ValueIterator<decltype(PtrMap)>;
        using const_iterator = ValueIterator<const decltype(PtrMap)>;

        inline iterator begin() { return iterator(PtrMap.begin()); }
        inline iterator end() { return iterator(PtrMap.end()); }

        inline auto pairs_begin() { return PtrMap.begin(); }
        inline auto pairs_end() { return PtrMap.end(); }

        inline ObjectInstance* at(void* ptr) { return PtrMap.at(ptr); }

        inline void add(ObjectInstance* instance)
        {
            PtrMap[instance->Memory] = instance;
        }

        inline void erase(ObjectInstance* instance)
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
		ApplicationDomain* const applicationDomain;
		uint64_t objectsCounter = 0;
        std::unordered_map<FieldSymbol*, ObjectInstance*> staticFields;
        
    public:
        static ObjectInstance* NullInstance;
        InstancesHeap Heap;
		
        GarbageCollector(ApplicationDomain* domain);

        ObjectInstance* FromValue(int64_t value);
        ObjectInstance* FromValue(double value);
        ObjectInstance* FromValue(bool value);
        ObjectInstance* FromValue(wchar_t value);
        ObjectInstance* FromValue(const wchar_t* value, bool isTransient);
        ObjectInstance* FromValue(const std::wstring& value);

        ObjectInstance* GetStaticField(FieldSymbol* field);
        void SetStaticField(FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo);
        ObjectInstance* CopyInstance(ObjectInstance* instance);
		
        void CollectInstance(ObjectInstance* instance);
        void DestroyInstance(ObjectInstance* instance);
        void TerminateInstance(ObjectInstance* instance);
		void Terminate();
	};
}
