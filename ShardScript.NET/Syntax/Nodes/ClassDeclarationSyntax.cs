using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Nodes;

public class ClassDeclarationSyntax
{
    public IntPtr Handle { get; }

    public string Name
    {
        get
        {
            var ptr = ShardScriptAPI.Shard_GetTypeName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public ClassDeclarationSyntax(IntPtr handle)
    {
        Handle = handle;
    }

    public IReadOnlyList<MethodSymbol> GetMethods(CompilationContext context)
    {
        int count = ShardScriptAPI.Shard_GetTypeMethodCount(context.Handle, Handle);
        MethodSymbol[] methods = new MethodSymbol[count];
        for (int i = 0; i < count; i++)
            methods[i] = new MethodSymbol(ShardScriptAPI.Shard_GetTypeMethod(context.Handle, Handle, i));

        return methods;
    }
}
