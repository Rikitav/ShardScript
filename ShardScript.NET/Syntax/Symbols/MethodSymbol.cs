using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Symbols;

public sealed class MethodSymbol : SyntaxSymbol
{
    private ShardManagedMethodCallbackNative? _nativeCallback;

    public bool IsStatic => NativeMethods.Shard_IsMethodStatic(Handle) != 0;

    public string Name
    {
        get
        {
            var ptr = NativeMethods.Shard_GetMethodName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public TypeSymbol? ReturnType
    {
        get
        {
            var type = NativeMethods.Shard_GetMethodReturnType(Handle);
            return type == IntPtr.Zero ? null : new TypeSymbol(type);
        }
    }

    public int ParameterCount => NativeMethods.Shard_GetMethodParameterCount(Handle);

    public MethodSymbol(IntPtr handle) : base(handle)
    {
    }

    public string GetParameterName(int index)
    {
        var ptr = NativeMethods.Shard_GetMethodParameterName(Handle, index);
        return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
    }

    public TypeSymbol? GetParameterType(int index)
    {
        var type = NativeMethods.Shard_GetMethodParameterType(Handle, index);
        return type == IntPtr.Zero ? null : new TypeSymbol(type);
    }

    internal void KeepCallbackAlive(ShardManagedMethodCallbackNative nativeCallback)
    {
        _nativeCallback = nativeCallback;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetMethodName(IntPtr method);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetMethodParameterCount(IntPtr method);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetMethodParameterName(IntPtr method, int index);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetMethodParameterType(IntPtr method, int index);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetMethodReturnType(IntPtr method);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_IsMethodStatic(IntPtr method);
    }
}
