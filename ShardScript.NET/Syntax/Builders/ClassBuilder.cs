using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class ClassBuilder : ISymbolBuilder<TypeSymbol>, IAccessible<ClassBuilder>, ILinkable<ClassBuilder>
{
    private readonly CompilationContext _context;
    private readonly NamespaceSymbol? _parentNamespace;
    private readonly string _name;

    public IntPtr Handle { get; }
    public TypeSymbol Symbol { get; }

    public ClassBuilder(CompilationContext context, NamespaceSymbol? parentNamespace, string name)
    {
        _context = context ?? throw new ArgumentNullException(nameof(context));
        _parentNamespace = parentNamespace;
        _name = name ?? throw new ArgumentNullException(nameof(name));

        Handle = NativeMethods.Shard_CreateClassSymbol(context.Handle, parentNamespace?.Handle ?? IntPtr.Zero, name);
        Symbol = new TypeSymbol(Handle);
    }

    public MethodBuilder Method(string name, TypeSymbol returnType)
        => new MethodBuilder(_context, Symbol, name, returnType);

    public FieldBuilder Field(string name, TypeSymbol type)
        => new FieldBuilder(_context, Symbol, name, type);

    public TypeSymbol Build() => Symbol;

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateClassSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);
    }
}
