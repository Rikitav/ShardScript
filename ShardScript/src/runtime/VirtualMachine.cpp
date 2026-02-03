#include <shard/runtime/VirtualMachine.h>

#include <vector>

using namespace shard;

template <typename T>
static T& ReadUnaligned(const std::vector<std::byte>& code, size_t& ip)
{
    T value{};
    std::memcpy(&value, &code[ip], sizeof(T));
    ip += sizeof(T);
    return value;
}

void VirtualMachine::Run()
{
}
