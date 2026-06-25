using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Symbols;

public class FieldSymbol : SyntaxSymbol
{
    public FieldSymbol(IntPtr handle) : base(handle) { }

    public string Name
    {
        get
        {
            IntPtr ptr = NativeMethods.Shard_GetFieldName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public TypeSymbol FieldType => new(NativeMethods.Shard_GetFieldType(Handle));

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetFieldName(IntPtr field);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetFieldType(IntPtr field);
    }
}
