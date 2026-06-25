using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Runtime;

/// <summary>
/// Provides access to the ShardScript virtual machine for invoking methods and running the entry point.
/// </summary>
public sealed class VirtualMachine
{
    private readonly IntPtr _handle;

    public IntPtr Handle => _handle;

    public VirtualMachine(IntPtr handle)
    {
        _handle = handle;
    }

    public void Run()
    {
        int result = NativeMethods.Shard_VMRun(_handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to run VM: {ShardEngineException.GetLastErrorMessage()}");
    }

    public void Abort()
    {
        NativeMethods.Shard_VMAbort(_handle);
    }

    public void TerminateCallStack()
    {
        NativeMethods.Shard_VMTerminateCallStack(_handle);
    }

    public ObjectInstance InvokeMethod(MethodSymbol method, params ObjectInstance[] args)
    {
        if (method == null)
            throw new ArgumentNullException(nameof(method));

        IntPtr[] argHandles = new IntPtr[args.Length];
        for (int i = 0; i < args.Length; i++)
            argHandles[i] = args[i].Handle;

        IntPtr result = NativeMethods.Shard_VMInvokeMethod(_handle, method.Handle, argHandles, argHandles.Length);
        return new ObjectInstance(result);
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_VMRun(IntPtr vm);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_VMAbort(IntPtr vm);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_VMTerminateCallStack(IntPtr vm);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_VMInvokeMethod(IntPtr vm, IntPtr method, IntPtr[] args, int argCount);
    }
}
