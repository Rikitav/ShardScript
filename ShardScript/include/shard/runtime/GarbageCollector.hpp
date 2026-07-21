#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/TypeShapeCache.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <cstdint>

namespace shard
{
    constexpr std::size_t SMALL_INTS_CACHE_MIN = -5;
    constexpr std::size_t SMALL_INTS_CACHE_MAX = 256;
    constexpr std::size_t SMALL_INTS_CACHE_SIZE = SMALL_INTS_CACHE_MAX - SMALL_INTS_CACHE_MIN;

    class ApplicationDomain;

	class SHARD_API InstancesHeap
	{
    private:
        std::vector<std::unique_ptr<ObjectInstance>> Instances;

    public:
        InstancesHeap() = default;

        InstancesHeap(const InstancesHeap&) = delete;
        InstancesHeap(InstancesHeap&&) = default;

        InstancesHeap& operator=(const InstancesHeap&) = delete;
        InstancesHeap& operator=(InstancesHeap&&) = default;

        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = ObjectInstance*;
            using difference_type = std::ptrdiff_t;
            using pointer = ObjectInstance**;
            using reference = ObjectInstance*&;

            iterator(std::vector<std::unique_ptr<ObjectInstance>>::iterator it) : it(it) {}

            ObjectInstance* operator*() const { return it->get(); }
            ObjectInstance* operator->() const { return it->get(); }

            iterator& operator++() { ++it; return *this; }
            iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }

            bool operator==(const iterator& other) const { return it == other.it; }
            bool operator!=(const iterator& other) const { return it != other.it; }

        private:
            std::vector<std::unique_ptr<ObjectInstance>>::iterator it;
        };

        using const_iterator = iterator;

        inline iterator begin() { return iterator(Instances.begin()); }
        inline iterator end() { return iterator(Instances.end()); }

        inline void add(ObjectInstance* instance)
        {
            Instances.emplace_back(instance);
        }

        inline void erase(ObjectInstance* instance)
        {
            auto it = std::find_if(Instances.begin(), Instances.end(),
                [instance](const std::unique_ptr<ObjectInstance>& entry) { return entry.get() == instance; });

            if (it != Instances.end())
            {
                it->release();
                Instances.erase(it);
            }
        }

        inline void clear()
        {
            for (auto& entry : Instances)
                entry.release();

            Instances.clear();
        }

        inline std::size_t size() const
        {
            return Instances.size();
        }
	};

	class SHARD_API GarbageCollector
	{
        static ObjectInstance* SmallInts;
        static std::int64_t* SmallIntsVals;

		ApplicationDomain* applicationDomain;
		std::uint64_t objectsCounter = 0;
        std::unordered_map<FieldSymbol*, ObjectInstance*> staticFields;
		std::vector<std::unique_ptr<ArrayTypeSymbol>> dynamicArrayTypes;
		std::vector<std::unique_ptr<TypeShape>> dynamicArrayShapes;

		TypeShapeCache& GetTypeShapeCache() const;

    public:
        static ObjectInstance* NullInstance;

        InstancesHeap Heap;

        GarbageCollector(ApplicationDomain* domain);

        GarbageCollector(const GarbageCollector&) = delete;
        GarbageCollector& operator=(const GarbageCollector&) = delete;

        GarbageCollector(GarbageCollector&&) = default;
        GarbageCollector& operator=(GarbageCollector&&) = default;

        ObjectInstance* FromValue(std::int64_t value);
        ObjectInstance* FromValue(std::uint8_t value);
        ObjectInstance* FromValue(double value);
        ObjectInstance* FromValue(bool value);
        ObjectInstance* FromValue(wchar_t value);
        //ObjectInstance* FromValue(const char* value);
        ObjectInstance* FromValue(const wchar_t* value, bool isTransient);
        ObjectInstance* FromValue(const std::wstring& value);
        
        ObjectInstance* FromNint(void* rawMemory, bool isTransient);

        ObjectInstance* GetStaticField(FieldSymbol* field);
        void SetStaticField(FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* AllocateInstance(TypeShape* shape, bool isTransient = false);
		ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo, bool isTransient = false);
		ObjectInstance* AllocateGeneric(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs, bool isTransient = false);
		ObjectInstance* AllocateArray(TypeSymbol* elementType, std::size_t length, bool isTransient = false);
        ObjectInstance* CopyInstance(ObjectInstance* instance);

        void CollectInstance(ObjectInstance* instance);
        void DestroyInstance(ObjectInstance* instance);
        void TerminateInstance(ObjectInstance* instance);
		void Terminate();
	};
}
