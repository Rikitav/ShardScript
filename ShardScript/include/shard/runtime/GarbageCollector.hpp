#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/TypeShapeCache.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <span>
#include <cstdint>

namespace shard
{
    class ApplicationDomain;
    class CallStackFrame;

	class SHARD_API InstancesHeap
	{
    private:
        std::unordered_set<ObjectInstance*> Instances;

    public:
        InstancesHeap() = default;

        InstancesHeap(const InstancesHeap&) = delete;
        InstancesHeap(InstancesHeap&&) = default;

        InstancesHeap& operator=(const InstancesHeap&) = delete;
        InstancesHeap& operator=(InstancesHeap&&) = default;

        using iterator = std::unordered_set<ObjectInstance*>::iterator;
        using const_iterator = iterator;

        inline iterator begin() { return Instances.begin(); }
        inline iterator end() { return Instances.end(); }

        inline void add(ObjectInstance* instance)
        {
            Instances.insert(instance);
        }

        inline void erase(ObjectInstance* instance)
        {
            Instances.erase(instance);
        }

        inline void clear()
        {
            Instances.clear();
        }

        inline std::size_t size() const
        {
            return Instances.size();
        }
	};

	class SHARD_API GarbageCollector
	{

		ApplicationDomain* applicationDomain;
		std::uint64_t objectsCounter = 0;
        std::unordered_map<FieldSymbol*, ObjectInstance*> staticFields;
		std::vector<std::unique_ptr<ArrayTypeSymbol>> dynamicArrayTypes;
		std::vector<std::unique_ptr<TypeShape>> dynamicArrayShapes;
        std::unordered_map<const wchar_t*, ObjectInstance*> internedStrings;

        struct AsyncRecord
        {
            bool IsTaskLike = false;
            bool IsFireAndForget = false;
            std::shared_ptr<CallStackFrame> FrameOwner;
            void* NativeState = nullptr;
        };

        std::unordered_map<ObjectInstance*, AsyncRecord> asyncTable;

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
        ObjectInstance* FromValue(const wchar_t* value);
        ObjectInstance* FromValue(const std::wstring& value);

        ObjectInstance* FromNint(std::intptr_t rawMemory);
        ObjectInstance* FromNint(std::uintptr_t rawMemory);
        ObjectInstance* FromNint(void* rawMemory);

        ObjectInstance* GetStaticField(FieldSymbol* field);
        void SetStaticField(FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* AllocateInstance(TypeShape* shape);
		ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo);
        ObjectInstance* AllocateGeneric(TypeSymbol* baseType, const std::span<TypeSymbol*> genericArgs);
		ObjectInstance* AllocateGeneric(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs);
		ObjectInstance* AllocateArray(TypeSymbol* elementType, std::size_t length);
        ObjectInstance* CopyInstance(ObjectInstance* instance);

        ObjectInstance* CreateView(const TypeSymbol* info, TypeShape* shape);
        ObjectInstance* InternString(const wchar_t* value);

        [[nodiscard]] bool IsHeapBacked(ObjectInstance* instance);
        ObjectInstance* Materialize(ObjectInstance* value);

        [[nodiscard]] bool IsTaskLike(ObjectInstance* instance);
        void MarkTaskLike(ObjectInstance* instance);
        [[nodiscard]] bool IsFireAndForget(ObjectInstance* instance);
        void MarkFireAndForget(ObjectInstance* instance);
        [[nodiscard]] void* GetAsyncNativeState(ObjectInstance* instance);
        void SetAsyncNativeState(ObjectInstance* instance, void* state);
        [[nodiscard]] std::shared_ptr<CallStackFrame> GetFrameOwner(ObjectInstance* instance);
        void BindToFrame(ObjectInstance* instance, std::shared_ptr<CallStackFrame> frame);
        void ReleaseFrameOwner(ObjectInstance* instance);

        void CollectInstance(ObjectInstance* instance);
        void DestroyInstance(ObjectInstance* instance);
        void TerminateInstance(ObjectInstance* instance, bool deleteInstance = true);
        void DeleteInstanceMemory(ObjectInstance* instance);
		void Terminate();
	};
}
