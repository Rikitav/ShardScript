using ShardScript.Application;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Nodes;

public class ClassDeclarationSyntax
{
    public IntPtr Handle { get; }

    public string Name
    {
        get
        {
            var ptr = NativeMethods.Shard_GetTypeName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public ClassDeclarationSyntax(IntPtr handle)
    {
        Handle = handle;
    }

    public IReadOnlyList<MethodSymbol> GetMethods(CompilationContext context)
    {
        int count = NativeMethods.Shard_GetTypeMethodCount(context.Handle, Handle);
        MethodSymbol[] methods = new MethodSymbol[count];
        for (int i = 0; i < count; i++)
            methods[i] = new MethodSymbol(NativeMethods.Shard_GetTypeMethod(context.Handle, Handle, i));

        return methods;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetTypeName(IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetTypeMethodCount(IntPtr ctx, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetTypeMethod(IntPtr ctx, IntPtr type, int index);
    }
}
