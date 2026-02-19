#include <shard/runtime/VirtualMachine.h>

#define SHARD_EXPORT extern "C" __declspec(dllexport)

using namespace shard;

SHARD_EXPORT void* VirtualMachine_Create(void* programImagePtr)
{
    if (!programImagePtr)
        return nullptr;

    ProgramVirtualImage* program = static_cast<ProgramVirtualImage*>(programImagePtr);
    VirtualMachine* vm = new VirtualMachine(*program);

    return static_cast<void*>(vm);
}
