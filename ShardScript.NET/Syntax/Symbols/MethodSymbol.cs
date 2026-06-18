using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Symbols;

public sealed class MethodSymbol : SyntaxSymbol
{
    private ShardManagedMethodCallbackNative? _nativeCallback;

    public bool IsStatic => ShardScriptAPI.Shard_IsMethodStatic(Handle) != 0;

    public string Name
    {
        get
        {
            var ptr = ShardScriptAPI.Shard_GetMethodName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public TypeSymbol? ReturnType
    {
        get
        {
            var type = ShardScriptAPI.Shard_GetMethodReturnType(Handle);
            return type == IntPtr.Zero ? null : new TypeSymbol(type);
        }
    }

    public int ParameterCount => ShardScriptAPI.Shard_GetMethodParameterCount(Handle);

    public MethodSymbol(IntPtr handle) : base(handle)
    {
    }

    public string GetParameterName(int index)
    {
        var ptr = ShardScriptAPI.Shard_GetMethodParameterName(Handle, index);
        return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
    }

    public TypeSymbol? GetParameterType(int index)
    {
        var type = ShardScriptAPI.Shard_GetMethodParameterType(Handle, index);
        return type == IntPtr.Zero ? null : new TypeSymbol(type);
    }

    internal void KeepCallbackAlive(ShardManagedMethodCallbackNative nativeCallback)
    {
        _nativeCallback = nativeCallback;
    }
}
