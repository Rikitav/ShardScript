using ShardScript.Application;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Builders;

public sealed class ClassBuilder : ISymbolBuilder<TypeSymbol>, IAccessible, ILinkable
{
    public CompilationContext Context { get; }
    public IntPtr Handle { get; }
    public TypeSymbol Symbol { get; }

    public ClassBuilder(CompilationContext context, NamespaceSymbol? parentNamespace, string name)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        Handle = NativeMethods.Shard_CreateClassSymbol(context.Handle, parentNamespace?.Handle ?? IntPtr.Zero, name);
        Symbol = new TypeSymbol(Handle);
    }

    public MethodBuilder Method(string name, TypeSymbol returnType)
        => new MethodBuilder(Context, Symbol, name, returnType);

    public FieldBuilder Field(string name, TypeSymbol type)
        => new FieldBuilder(Context, Symbol, name, type);

    public TypeSymbol Build() => Symbol;

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateClassSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);
    }
}
