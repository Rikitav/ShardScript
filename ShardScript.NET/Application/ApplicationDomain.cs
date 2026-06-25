using ShardScript.NET.Runtime;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Application;

/// <summary>
/// Represents a compiled ShardScript application that can be executed by the virtual machine.
/// </summary>
public sealed class ApplicationDomain : IDisposable
{
    private IntPtr _handle;

    public IntPtr Handle => _handle;

    public VirtualMachine VirtualMachine
    {
        get
        {
            IntPtr vm = NativeMethods.Shard_GetVirtualMachine(_handle);
            if (vm == IntPtr.Zero)
                throw new InvalidOperationException($"Failed to get virtual machine: {ShardEngineException.GetLastErrorMessage()}");

            return new VirtualMachine(vm);
        }
    }

    public GarbageCollector GarbageCollector
    {
        get
        {
            IntPtr gc = NativeMethods.Shard_GetGarbageCollector(_handle);
            if (gc == IntPtr.Zero)
                throw new InvalidOperationException($"Failed to get garbage collector: {ShardEngineException.GetLastErrorMessage()}");

            return new GarbageCollector(gc);
        }
    }

    public MethodSymbol? EntryPointMethod
    {
        get
        {
            IntPtr method = NativeMethods.Shard_GetEntryPointMethod(_handle);
            return method == IntPtr.Zero ? null : new MethodSymbol(method);
        }
    }

    public ApplicationDomain(IntPtr handle)
    {
        _handle = handle;
    }

    public void Run()
    {
        int result = NativeMethods.Shard_RunDomain(_handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to run domain: {ShardEngineException.GetLastErrorMessage()}");
    }

    public void Dispose()
    {
        if (_handle != IntPtr.Zero)
        {
            NativeMethods.Shard_DestroyDomain(_handle);
            _handle = IntPtr.Zero;
        }
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_RunDomain(IntPtr domain);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_DestroyDomain(IntPtr domain);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetVirtualMachine(IntPtr domain);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetGarbageCollector(IntPtr domain);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetProgram(IntPtr domain);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetEntryPointMethod(IntPtr domain);
    }
}
