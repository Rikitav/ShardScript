using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class NamespaceBuilder : ISymbolBuilder<NamespaceSymbol>, IAccessible, ILinkable
{
    private readonly NamespaceSymbol? _parent;
    private readonly string _name;

    public CompilationContext Context { get; }
    public IntPtr Handle { get; }
    public NamespaceSymbol Symbol { get; }

    public NamespaceBuilder(CompilationContext context, string name, NamespaceSymbol? parent = null)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        _name = name ?? throw new ArgumentNullException(nameof(name));
        _parent = parent;

        Handle = NativeMethods.Shard_CreateNamespaceSymbol(context.Handle, parent?.Handle ?? IntPtr.Zero, name);
        Symbol = new NamespaceSymbol(Handle);
    }

    public NamespaceBuilder Public()
    {
        ShardEngineException.ThrowIfError(NativeMethods.Shard_SetSymbolAccesibility(Handle, (int)SymbolAccessibility.Public));
        return this;
    }

    public NamespaceBuilder Private()
    {
        ShardEngineException.ThrowIfError(NativeMethods.Shard_SetSymbolAccesibility(Handle, (int)SymbolAccessibility.Private));
        return this;
    }

    public ClassBuilder Class(string name) => new(Context, Symbol, name);

    public NamespaceBuilder Namespace(string name) => new(Context, name, Symbol);

    public NamespaceSymbol Build() => Symbol;

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateNamespaceSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetSymbolAccesibility(IntPtr symbol, int accessibility);
    }
}
