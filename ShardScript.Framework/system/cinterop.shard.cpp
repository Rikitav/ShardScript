#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

using namespace shard;

// ============================================================================
//  Local helpers
// ============================================================================

namespace
{
    inline std::byte* PtrAt(void* base, std::int64_t offset)
    {
        return static_cast<std::byte*>(base) + offset;
    }

    // Narrow an ASCII string (C symbol name / library path) for GetProcAddress,
    // dlopen and dlsym, which all expect char-based strings.
    inline std::string ToNarrow(const wchar_t* wide)
    {
        if (wide == nullptr)
            return {};

        std::string narrow;
        for (const wchar_t* p = wide; *p != L'\0'; ++p)
            narrow.push_back(static_cast<char>(static_cast<unsigned char>(*p & 0xFF)));
        return narrow;
    }

    inline void* LibLoad(const wchar_t* path)
    {
#ifdef _WIN32
        return static_cast<void*>(::LoadLibraryW(path));
#else
        std::string narrow = ToNarrow(path);
        return ::dlopen(narrow.empty() ? nullptr : narrow.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
    }

    inline void* LibSymbol(void* handle, const char* name)
    {
#ifdef _WIN32
        return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), name));
#else
        // Clear any pending error so a real "not found" can be distinguished.
        ::dlerror();
        return ::dlsym(handle, name);
#endif
    }

    inline void LibFree(void* handle)
    {
        if (handle == nullptr)
            return;
#ifdef _WIN32
        ::FreeLibrary(static_cast<HMODULE>(handle));
#else
        ::dlclose(handle);
#endif
    }

    // Decode a NUL-terminated ANSI byte buffer into a wide string.
    inline std::wstring AnsiToWide(const char* bytes)
    {
        if (bytes == nullptr)
            return {};

#ifdef _WIN32
        int len = ::MultiByteToWideChar(CP_ACP, 0, bytes, -1, nullptr, 0);
        if (len <= 0)
            return {};

        std::wstring wide(static_cast<std::size_t>(len - 1), L'\0');
        ::MultiByteToWideChar(CP_ACP, 0, bytes, -1, wide.data(), len);
        return wide;
#else
        std::size_t len = std::strlen(bytes);
        std::wstring wide(len, L'\0');
        std::mbstowcs(wide.data(), bytes, wide.size());
        wide.resize(std::wcslen(wide.c_str()));
        return wide;
#endif
    }

    // Encode a wide string into an ANSI byte buffer (NUL-terminated).
    inline std::string WideToAnsi(const wchar_t* wide)
    {
        if (wide == nullptr)
            return {};

#ifdef _WIN32
        int len = ::WideCharToMultiByte(CP_ACP, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return {};

        std::string narrow(static_cast<std::size_t>(len - 1), '\0');
        ::WideCharToMultiByte(CP_ACP, 0, wide, -1, narrow.data(), len, nullptr, nullptr);
        return narrow;
#else
        std::size_t len = std::wcslen(wide);
        std::string narrow(len * 4 + 1, '\0');
        std::size_t written = std::wcstombs(narrow.data(), wide, narrow.size());
        if (written == static_cast<std::size_t>(-1))
            return {};
        narrow.resize(std::strlen(narrow.c_str()));
        return narrow;
#endif
    }

    // Read a NUL-terminated UTF-16 (2-byte code unit) buffer into a wide string.
    // This matches Win32 LPCWSTR semantics and stays portable to platforms whose
    // wchar_t is 32-bit.
    inline std::wstring Utf16ToWide(const void* ptr)
    {
        if (ptr == nullptr)
            return {};

        const auto* units = static_cast<const std::uint16_t*>(ptr);
        std::wstring wide;
        while (*units != 0u)
        {
            wide.push_back(static_cast<wchar_t>(*units));
            ++units;
        }
        return wide;
    }
}

// ============================================================================
//  NativeLibrary — dynamic loading & symbol resolution
// ============================================================================

static ObjectInstance* cinterop_NativeLibrary_Load(const CallState& context) noexcept(false)
{
    const wchar_t* path = context.Args[0]->AsString();
    void* handle = LibLoad(path);
    if (handle == nullptr)
    {
#ifdef _WIN32
        throw std::runtime_error("interop.NativeLibrary.Load: LoadLibraryW failed (GetLastError = "
            + std::to_string(::GetLastError()) + ")");
#else
        const char* err = ::dlerror();
        throw std::runtime_error(std::string("interop.NativeLibrary.Load: dlopen failed")
            + (err != nullptr ? (std::string(": ") + err) : std::string()));
#endif
    }

    return context.Collector.FromNint(handle, false);
}

static ObjectInstance* cinterop_NativeLibrary_GetFunction(const CallState& context) noexcept(false)
{
    void* handle = context.Args[0]->AsNint();
    const wchar_t* name = context.Args[1]->AsString();

    if (handle == nullptr)
        throw std::runtime_error("interop.NativeLibrary.GetFunction: null library handle");

    std::string narrowName = ToNarrow(name);
    void* symbol = LibSymbol(handle, narrowName.c_str());

    // On POSIX a NULL symbol may legitimately be a real export; on Windows a
    // NULL result is a definitive "not found". Either way the caller gets the
    // raw pointer (possibly 0) and can decide how to react.
    return context.Collector.FromNint(symbol, false);
}

static ObjectInstance* cinterop_NativeLibrary_Free(const CallState& context) noexcept
{
    void* handle = context.Args[0]->AsNint();
    LibFree(handle);
    return nullptr; // void
}

// ============================================================================
//  Marshal — native memory management
// ============================================================================

static ObjectInstance* cinterop_Marshal_Alloc(const CallState& context) noexcept(false)
{
    std::size_t size = static_cast<std::size_t>(context.Args[0]->AsInteger());
    void* block = std::malloc(size);
    
    if (block == nullptr && size != 0)
        throw std::runtime_error("interop.Marshal.Alloc: out of memory");

    return context.Collector.FromNint(block, false);
}

static ObjectInstance* cinterop_Marshal_AllocZeroed(const CallState& context) noexcept(false)
{
    std::size_t count = static_cast<std::size_t>(context.Args[0]->AsInteger());
    std::size_t size = static_cast<std::size_t>(context.Args[1]->AsInteger());
    void* block = std::calloc(count == 0 ? 1 : count, size == 0 ? 1 : size);

    if (block == nullptr && (count != 0 && size != 0))
        throw std::runtime_error("interop.Marshal.AllocZeroed: out of memory");

    return context.Collector.FromNint(block, false);
}

static ObjectInstance* cinterop_Marshal_Realloc(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::size_t size = static_cast<std::size_t>(context.Args[1]->AsInteger());

    void* block = std::realloc(ptr, size);
    if (block == nullptr && size != 0)
        throw std::runtime_error("interop.Marshal.Realloc: out of memory");

    return context.Collector.FromNint(block, false);
}

static ObjectInstance* cinterop_Marshal_Free(const CallState& context) noexcept
{
    void* ptr = context.Args[0]->AsNint();
    std::free(ptr);
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_Copy(const CallState& context) noexcept(false)
{
    void* source = context.Args[0]->AsNint();
    void* dest = context.Args[1]->AsNint();
    std::size_t length = static_cast<std::size_t>(context.Args[2]->AsInteger());

    if (length != 0 && (source == nullptr || dest == nullptr))
        throw std::runtime_error("interop.Marshal.Copy: null pointer");

    std::memmove(dest, source, length); // memmove: regions may overlap
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_Fill(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::size_t length = static_cast<std::size_t>(context.Args[1]->AsInteger());
    int value = static_cast<int>(context.Args[2]->AsInteger());

    if (length != 0 && ptr == nullptr)
        throw std::runtime_error("interop.Marshal.Fill: null pointer");

    std::memset(ptr, value & 0xFF, length);
    return nullptr; // void
}

// ----------------------------------------------------------------------------
//  Marshal — typed reads / writes
// ----------------------------------------------------------------------------

static ObjectInstance* cinterop_Marshal_ReadByte(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadByte: null pointer");

    std::uint8_t value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(static_cast<std::int64_t>(value));
}

static ObjectInstance* cinterop_Marshal_ReadInt16(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadInt16: null pointer");

    std::int16_t value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(static_cast<std::int64_t>(value));
}

static ObjectInstance* cinterop_Marshal_ReadInt32(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadInt32: null pointer");

    std::int32_t value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(static_cast<std::int64_t>(value));
}

static ObjectInstance* cinterop_Marshal_ReadInt64(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadInt64: null pointer");

    std::int64_t value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(value);
}

static ObjectInstance* cinterop_Marshal_ReadIntPtr(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadIntPtr: null pointer");

    void* value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromNint(value, false);
}

static ObjectInstance* cinterop_Marshal_ReadFloat(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadFloat: null pointer");

    float value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(static_cast<double>(value));
}

static ObjectInstance* cinterop_Marshal_ReadDouble(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.ReadDouble: null pointer");

    double value;
    std::memcpy(&value, PtrAt(ptr, offset), sizeof(value));
    return context.Collector.FromValue(value);
}

static ObjectInstance* cinterop_Marshal_WriteByte(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    std::uint8_t value = static_cast<std::uint8_t>(context.Args[2]->AsInteger());
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteByte: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteInt16(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    std::int16_t value = static_cast<std::int16_t>(context.Args[2]->AsInteger());
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteInt16: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteInt32(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    std::int32_t value = static_cast<std::int32_t>(context.Args[2]->AsInteger());
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteInt32: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteInt64(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    std::int64_t value = context.Args[2]->AsInteger();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteInt64: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteIntPtr(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    void* value = context.Args[2]->AsNint();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteIntPtr: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteFloat(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    float value = static_cast<float>(context.Args[2]->AsDouble());
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteFloat: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

static ObjectInstance* cinterop_Marshal_WriteDouble(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    double value = context.Args[2]->AsDouble();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.WriteDouble: null pointer");

    std::memcpy(PtrAt(ptr, offset), &value, sizeof(value));
    return nullptr; // void
}

// ----------------------------------------------------------------------------
//  Marshal — string marshalling
// ----------------------------------------------------------------------------

static ObjectInstance* cinterop_Marshal_StringFromAnsi(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.StringFromAnsi: null pointer");

    return context.Collector.FromValue(AnsiToWide(static_cast<const char*>(ptr)));
}

static ObjectInstance* cinterop_Marshal_StringFromUnicode(const CallState& context) noexcept(false)
{
    void* ptr = context.Args[0]->AsNint();
    if (ptr == nullptr)
        throw std::runtime_error("interop.Marshal.StringFromUnicode: null pointer");

    return context.Collector.FromValue(Utf16ToWide(ptr));
}

static ObjectInstance* cinterop_Marshal_StringToAnsi(const CallState& context) noexcept(false)
{
    const wchar_t* value = context.Args[0]->AsString();
    std::string narrow = WideToAnsi(value);

    // NUL-terminated buffer owned by the caller; free with Marshal.Free.
    char* block = static_cast<char*>(std::malloc(narrow.size() + 1));
    if (block == nullptr)
        throw std::runtime_error("interop.Marshal.StringToAnsi: out of memory");

    if (!narrow.empty())
        std::memcpy(block, narrow.data(), narrow.size());
    block[narrow.size()] = '\0';

    return context.Collector.FromNint(block, false);
}

static ObjectInstance* cinterop_Marshal_StringToUnicode(const CallState& context) noexcept(false)
{
    const wchar_t* value = context.Args[0]->AsString();
    std::size_t units = std::wcslen(value);

    // NUL-terminated UTF-16 buffer owned by the caller; free with Marshal.Free.
    auto* block = static_cast<std::uint16_t*>(std::malloc((units + 1) * sizeof(std::uint16_t)));
    if (block == nullptr)
        throw std::runtime_error("interop.Marshal.StringToUnicode: out of memory");

    for (std::size_t i = 0; i < units; ++i)
        block[i] = static_cast<std::uint16_t>(value[i]);
    block[units] = 0;

    return context.Collector.FromNint(block, false);
}

// ----------------------------------------------------------------------------
//  Marshal — pointer arithmetic
// ----------------------------------------------------------------------------

static ObjectInstance* cinterop_Marshal_Add(const CallState& context) noexcept
{
    void* ptr = context.Args[0]->AsNint();
    std::int64_t offset = context.Args[1]->AsInteger();
    return context.Collector.FromNint(PtrAt(ptr, offset), false);
}

static ObjectInstance* cinterop_Marshal_IntPtrSize_get(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(sizeof(void*)));
}

// ============================================================================
//  NativeCall — invoke raw C function pointers
//
//  Arguments are declared `any`, so a single call site can mix integers and
//  pointers. Each argument is read as a raw 64-bit word (int and nint share the
//  same inline layout), which on 64-bit ABIs is exactly what the callee receives
//  in its integer/pointer argument registers. The return family must match the C
//  function's return type so the result register is read at the correct width:
//
//     Call        -> C function returns int            (32-bit, sign-extended)
//     CallI64     -> C function returns a 64-bit int    (long long / intptr_t)
//     CallN       -> C function returns a pointer       (void*)
//     CallVoid    -> C function returns void
//     CallDouble  -> C function takes & returns double  (vector-register ABI)
//
//  Each family is overloaded for 0..4 arguments.
// ============================================================================

// Read an int or nint argument as a raw 64-bit word. Both store their value at
// offset 0, so this works for either and lets FFI call sites pass integers and
// pointers interchangeably through `any` parameters.
inline std::int64_t cinterop_ReadWord(ObjectInstance* arg)
{
    if (arg == GarbageCollector::NullInstance)
        return 0;

    std::int64_t word = 0;
    std::memcpy(&word, arg->getMemory(), sizeof(word));
    return word;
}

template <typename Ret, typename... Args>
inline Ret cinterop_InvokeNative(void* fn, Args... args)
{
    using FuncT = Ret(*)(Args...);
    return reinterpret_cast<FuncT>(fn)(args...);
}

static ObjectInstance* cinterop_NativeCall_Call(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.Call: null function pointer");

    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int32_t>(fn)));
}

static ObjectInstance* cinterop_NativeCall_Call1(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.Call: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int32_t, std::int64_t>(fn, a)));
}

static ObjectInstance* cinterop_NativeCall_Call2(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.Call: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int32_t, std::int64_t, std::int64_t>(fn, a, b)));
}

static ObjectInstance* cinterop_NativeCall_Call3(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.Call: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int32_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c)));
}

static ObjectInstance* cinterop_NativeCall_Call4(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.Call: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    std::int64_t d = cinterop_ReadWord(context.Args[4]);
    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int32_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c, d)));
}

static ObjectInstance* cinterop_NativeCall_CallI64(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallI64: null function pointer");

    return context.Collector.FromValue(static_cast<std::int64_t>(cinterop_InvokeNative<std::int64_t>(fn)));
}

static ObjectInstance* cinterop_NativeCall_CallI64_1(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallI64: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    return context.Collector.FromValue(cinterop_InvokeNative<std::int64_t, std::int64_t>(fn, a));
}

static ObjectInstance* cinterop_NativeCall_CallI64_2(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallI64: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    return context.Collector.FromValue(cinterop_InvokeNative<std::int64_t, std::int64_t, std::int64_t>(fn, a, b));
}

static ObjectInstance* cinterop_NativeCall_CallI64_3(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallI64: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    return context.Collector.FromValue(cinterop_InvokeNative<std::int64_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c));
}

static ObjectInstance* cinterop_NativeCall_CallI64_4(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallI64: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    std::int64_t d = cinterop_ReadWord(context.Args[4]);
    return context.Collector.FromValue(cinterop_InvokeNative<std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c, d));
}

static ObjectInstance* cinterop_NativeCall_CallN(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallN: null function pointer");

    return context.Collector.FromNint(cinterop_InvokeNative<void*>(fn), false);
}

static ObjectInstance* cinterop_NativeCall_CallN1(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallN: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    return context.Collector.FromNint(cinterop_InvokeNative<void*, std::int64_t>(fn, a), false);
}

static ObjectInstance* cinterop_NativeCall_CallN2(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallN: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    return context.Collector.FromNint(cinterop_InvokeNative<void*, std::int64_t, std::int64_t>(fn, a, b), false);
}

static ObjectInstance* cinterop_NativeCall_CallN3(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallN: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    return context.Collector.FromNint(cinterop_InvokeNative<void*, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c), false);
}

static ObjectInstance* cinterop_NativeCall_CallN4(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallN: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    std::int64_t d = cinterop_ReadWord(context.Args[4]);
    return context.Collector.FromNint(cinterop_InvokeNative<void*, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c, d), false);
}

static ObjectInstance* cinterop_NativeCall_CallVoid(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallVoid: null function pointer");

    cinterop_InvokeNative<void>(fn);
    return nullptr; // void
}

static ObjectInstance* cinterop_NativeCall_CallVoid1(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallVoid: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    cinterop_InvokeNative<void, std::int64_t>(fn, a);
    return nullptr; // void
}

static ObjectInstance* cinterop_NativeCall_CallVoid2(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallVoid: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    cinterop_InvokeNative<void, std::int64_t, std::int64_t>(fn, a, b);
    return nullptr; // void
}

static ObjectInstance* cinterop_NativeCall_CallVoid3(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallVoid: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    cinterop_InvokeNative<void, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c);
    return nullptr; // void
}

static ObjectInstance* cinterop_NativeCall_CallVoid4(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallVoid: null function pointer");

    std::int64_t a = cinterop_ReadWord(context.Args[1]);
    std::int64_t b = cinterop_ReadWord(context.Args[2]);
    std::int64_t c = cinterop_ReadWord(context.Args[3]);
    std::int64_t d = cinterop_ReadWord(context.Args[4]);
    cinterop_InvokeNative<void, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(fn, a, b, c, d);
    return nullptr; // void
}

static ObjectInstance* cinterop_NativeCall_CallDouble(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallDouble: null function pointer");

    return context.Collector.FromValue(cinterop_InvokeNative<double>(fn));
}

static ObjectInstance* cinterop_NativeCall_CallDouble1(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallDouble: null function pointer");

    double a = context.Args[1]->AsDouble();
    return context.Collector.FromValue(cinterop_InvokeNative<double, double>(fn, a));
}

static ObjectInstance* cinterop_NativeCall_CallDouble2(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallDouble: null function pointer");

    double a = context.Args[1]->AsDouble();
    double b = context.Args[2]->AsDouble();
    return context.Collector.FromValue(cinterop_InvokeNative<double, double, double>(fn, a, b));
}

static ObjectInstance* cinterop_NativeCall_CallDouble3(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallDouble: null function pointer");

    double a = context.Args[1]->AsDouble();
    double b = context.Args[2]->AsDouble();
    double c = context.Args[3]->AsDouble();
    return context.Collector.FromValue(cinterop_InvokeNative<double, double, double, double>(fn, a, b, c));
}

static ObjectInstance* cinterop_NativeCall_CallDouble4(const CallState& context) noexcept(false)
{
    void* fn = context.Args[0]->AsNint();
    if (fn == nullptr)
        throw std::runtime_error("interop.NativeCall.CallDouble: null function pointer");

    double a = context.Args[1]->AsDouble();
    double b = context.Args[2]->AsDouble();
    double c = context.Args[3]->AsDouble();
    double d = context.Args[4]->AsDouble();
    return context.Collector.FromValue(cinterop_InvokeNative<double, double, double, double, double>(fn, a, b, c, d));
}

// ============================================================================
//  Library metadata & symbol registration
// ============================================================================

SHARDLIB_GETMETADATA
{
    lib.Name        = L"shard.cinterop";
    lib.Description = L"Native C interop: dynamic libraries, memory marshalling, FFI invocation";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> interopNamespace(context, L"interop");

    // ------------------------------------------------------------------
    //  class NativeLibrary (static)
    // ------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> nativeLibClass = interopNamespace.AddClass(L"NativeLibrary", LINK_STATIC);

    nativeLibClass.AddMethod(L"Load", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&cinterop_NativeLibrary_Load);

    nativeLibClass.AddMethod(L"GetFunction", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"handle", TYPE_NINT)
        .AddParameter(L"name", TYPE_STRING)
        .SetCallback(&cinterop_NativeLibrary_GetFunction);

    nativeLibClass.AddMethod(L"Free", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"handle", TYPE_NINT)
        .SetCallback(&cinterop_NativeLibrary_Free);

    // ------------------------------------------------------------------
    //  class Marshal (static) — memory
    // ------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> marshalClass = interopNamespace.AddClass(L"Marshal", LINK_STATIC);

    marshalClass.AddMethod(L"Alloc", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"size", TYPE_INT)
        .SetCallback(&cinterop_Marshal_Alloc);

    marshalClass.AddMethod(L"AllocZeroed", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"count", TYPE_INT)
        .AddParameter(L"size", TYPE_INT)
        .SetCallback(&cinterop_Marshal_AllocZeroed);

    marshalClass.AddMethod(L"Realloc", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"size", TYPE_INT)
        .SetCallback(&cinterop_Marshal_Realloc);

    marshalClass.AddMethod(L"Free", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .SetCallback(&cinterop_Marshal_Free);

    marshalClass.AddMethod(L"Copy", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"source", TYPE_NINT)
        .AddParameter(L"destination", TYPE_NINT)
        .AddParameter(L"length", TYPE_INT)
        .SetCallback(&cinterop_Marshal_Copy);

    marshalClass.AddMethod(L"Fill", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"length", TYPE_INT)
        .AddParameter(L"value", TYPE_INT)
        .SetCallback(&cinterop_Marshal_Fill);

    // --- Marshal — typed reads ---
    marshalClass.AddMethod(L"ReadByte", TYPE_INT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadByte);

    marshalClass.AddMethod(L"ReadInt16", TYPE_INT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadInt16);

    marshalClass.AddMethod(L"ReadInt32", TYPE_INT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadInt32);

    marshalClass.AddMethod(L"ReadInt64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadInt64);

    marshalClass.AddMethod(L"ReadIntPtr", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadIntPtr);

    marshalClass.AddMethod(L"ReadFloat", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadFloat);

    marshalClass.AddMethod(L"ReadDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_ReadDouble);

    // --- Marshal — typed writes ---
    marshalClass.AddMethod(L"WriteByte", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_INT)
        .SetCallback(&cinterop_Marshal_WriteByte);

    marshalClass.AddMethod(L"WriteInt16", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_INT)
        .SetCallback(&cinterop_Marshal_WriteInt16);

    marshalClass.AddMethod(L"WriteInt32", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_INT)
        .SetCallback(&cinterop_Marshal_WriteInt32);

    marshalClass.AddMethod(L"WriteInt64", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_INT)
        .SetCallback(&cinterop_Marshal_WriteInt64);

    marshalClass.AddMethod(L"WriteIntPtr", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_NINT)
        .SetCallback(&cinterop_Marshal_WriteIntPtr);

    marshalClass.AddMethod(L"WriteFloat", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_DOUBLE)
        .SetCallback(&cinterop_Marshal_WriteFloat);

    marshalClass.AddMethod(L"WriteDouble", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .AddParameter(L"value", TYPE_DOUBLE)
        .SetCallback(&cinterop_Marshal_WriteDouble);

    // --- Marshal — string marshalling ---
    marshalClass.AddMethod(L"StringFromAnsi", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .SetCallback(&cinterop_Marshal_StringFromAnsi);

    marshalClass.AddMethod(L"StringFromUnicode", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .SetCallback(&cinterop_Marshal_StringFromUnicode);

    marshalClass.AddMethod(L"StringToAnsi", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&cinterop_Marshal_StringToAnsi);

    marshalClass.AddMethod(L"StringToUnicode", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&cinterop_Marshal_StringToUnicode);

    // --- Marshal — pointer arithmetic ---
    marshalClass.AddMethod(L"Add", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"ptr", TYPE_NINT)
        .AddParameter(L"offset", TYPE_INT)
        .SetCallback(&cinterop_Marshal_Add);

    marshalClass.AddProperty(L"IntPtrSize", TYPE_INT, LINK_STATIC, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&cinterop_Marshal_IntPtrSize_get);

    // ------------------------------------------------------------------
    //  class NativeCall (static) — FFI invocation
    // ------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> nativeCallClass = interopNamespace.AddClass(L"NativeCall", LINK_STATIC);

    // Call -> C function returning int (32-bit, sign-extended)
    nativeCallClass.AddMethod(L"Call", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .SetCallback(&cinterop_NativeCall_Call);
    nativeCallClass.AddMethod(L"Call", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_Call1);
    nativeCallClass.AddMethod(L"Call", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_Call2);
    nativeCallClass.AddMethod(L"Call", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_Call3);
    nativeCallClass.AddMethod(L"Call", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .AddParameter(L"d", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_Call4);

    // CallI64 -> C function returning a 64-bit int
    nativeCallClass.AddMethod(L"CallI64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .SetCallback(&cinterop_NativeCall_CallI64);
    nativeCallClass.AddMethod(L"CallI64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallI64_1);
    nativeCallClass.AddMethod(L"CallI64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallI64_2);
    nativeCallClass.AddMethod(L"CallI64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallI64_3);
    nativeCallClass.AddMethod(L"CallI64", TYPE_INT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .AddParameter(L"d", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallI64_4);

    // CallN -> C function returning a pointer
    nativeCallClass.AddMethod(L"CallN", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .SetCallback(&cinterop_NativeCall_CallN);
    nativeCallClass.AddMethod(L"CallN", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallN1);
    nativeCallClass.AddMethod(L"CallN", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallN2);
    nativeCallClass.AddMethod(L"CallN", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallN3);
    nativeCallClass.AddMethod(L"CallN", TYPE_NINT, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .AddParameter(L"d", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallN4);

    // CallVoid -> C function returning void
    nativeCallClass.AddMethod(L"CallVoid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .SetCallback(&cinterop_NativeCall_CallVoid);
    nativeCallClass.AddMethod(L"CallVoid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallVoid1);
    nativeCallClass.AddMethod(L"CallVoid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallVoid2);
    nativeCallClass.AddMethod(L"CallVoid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallVoid3);
    nativeCallClass.AddMethod(L"CallVoid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_ANY)
        .AddParameter(L"b", TYPE_ANY)
        .AddParameter(L"c", TYPE_ANY)
        .AddParameter(L"d", TYPE_ANY)
        .SetCallback(&cinterop_NativeCall_CallVoid4);

    // CallDouble -> C function taking & returning double
    nativeCallClass.AddMethod(L"CallDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .SetCallback(&cinterop_NativeCall_CallDouble);
    nativeCallClass.AddMethod(L"CallDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_DOUBLE)
        .SetCallback(&cinterop_NativeCall_CallDouble1);
    nativeCallClass.AddMethod(L"CallDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_DOUBLE)
        .AddParameter(L"b", TYPE_DOUBLE)
        .SetCallback(&cinterop_NativeCall_CallDouble2);
    nativeCallClass.AddMethod(L"CallDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_DOUBLE)
        .AddParameter(L"b", TYPE_DOUBLE)
        .AddParameter(L"c", TYPE_DOUBLE)
        .SetCallback(&cinterop_NativeCall_CallDouble3);
    nativeCallClass.AddMethod(L"CallDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"function", TYPE_NINT)
        .AddParameter(L"a", TYPE_DOUBLE)
        .AddParameter(L"b", TYPE_DOUBLE)
        .AddParameter(L"c", TYPE_DOUBLE)
        .AddParameter(L"d", TYPE_DOUBLE)
        .SetCallback(&cinterop_NativeCall_CallDouble4);
}
