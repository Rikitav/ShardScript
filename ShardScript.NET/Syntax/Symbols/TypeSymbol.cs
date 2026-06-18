using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Symbols;

public sealed class TypeSymbol : SyntaxSymbol
{
    public TypeSymbol(IntPtr handle) : base(handle) { }

    public string Name
    {
        get
        {
            IntPtr ptr = NativeMethods.Shard_GetSymbolName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public MethodSymbol? FindMethod(string name, int parameterCount)
    {
        IntPtr method = NativeMethods.Shard_FindMethodInType(Handle, name, parameterCount);
        return method == IntPtr.Zero ? null : new MethodSymbol(method);
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_FindMethodInType(IntPtr type, [MarshalAs(UnmanagedType.LPWStr)] string name, int parameterCount);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetSymbolName(IntPtr symbol);
    }
}
