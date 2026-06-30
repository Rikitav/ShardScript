using ShardScript.Application;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Fluent builder for creating namespace symbols with lambda scoping.
/// </summary>
public sealed class NamespaceBuilder : IAccessible, IContainerBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the namespace symbol being built.
    /// </summary>
    public NamespaceSymbol Symbol { get; }

    /// <summary>
    /// Gets the parent namespace, if any.
    /// </summary>
    public NamespaceSymbol? Parent { get; }

    /// <summary>
    /// Gets the name of the namespace.
    /// </summary>
    public string Name { get; }

    IntPtr IAccessible.Handle => Symbol.Handle;
    SyntaxSymbol IContainerBuilder.Symbol => Symbol;

    internal NamespaceBuilder(CompilationContext context, string name, NamespaceSymbol? parent = null)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        Name = name ?? throw new ArgumentNullException(nameof(name));
        Parent = parent;

        IntPtr handle = NativeMethods.Shard_CreateNamespaceSymbol(
            context.Handle, parent?.Handle ?? IntPtr.Zero, name);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create namespace symbol '{name}'.");

        Symbol = new NamespaceSymbol(handle);
    }

    /// <summary>
    /// Creates a nested namespace with the specified configuration.
    /// </summary>
    public NamespaceBuilder WithNamespace(string name, Action<NamespaceBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        NamespaceBuilder nestedBuilder = new NamespaceBuilder(Context, name, Symbol);
        configure(nestedBuilder);
        return this;
    }

    /// <summary>
    /// Creates a class with the specified configuration.
    /// </summary>
    public NamespaceBuilder WithClass(string name, Action<ClassBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        ClassBuilder classBuilder = new ClassBuilder(Context, Symbol, name);
        configure(classBuilder);
        return this;
    }

    /// <summary>
    /// Creates a class with the specified configuration and extracts the symbol.
    /// </summary>
    public NamespaceBuilder WithClass(string name, Action<ClassBuilder> configure, out TypeSymbol symbol)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        ClassBuilder classBuilder = new ClassBuilder(Context, Symbol, name);
        configure(classBuilder);
        symbol = classBuilder.Symbol;
        return this;
    }

    /// <summary>
    /// Creates an enum with the specified configuration.
    /// Note: Programmatic enum creation is not currently supported via native API.
    /// </summary>
    public NamespaceBuilder WithEnum(string name, EnumKind kind, Action<EnumBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        EnumBuilder enumBuilder = new EnumBuilder(Context, Symbol, name, kind);
        configure(enumBuilder);
        enumBuilder.Build(); // Build to apply literals
        return this;
    }

    /// <summary>
    /// Creates an enum with the specified configuration and extracts the symbol.
    /// </summary>
    public NamespaceBuilder WithEnum(string name, EnumKind kind, Action<EnumBuilder> configure, out EnumSymbol symbol)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        EnumBuilder enumBuilder = new EnumBuilder(Context, Symbol, name, kind);
        configure(enumBuilder);
        symbol = enumBuilder.Build();
        return this;
    }

    /// <summary>
    /// Sets the namespace accessibility to public.
    /// </summary>
    public NamespaceBuilder Public()
    {
        SymbolBuilderExtensions.Public((IAccessible)this);
        return this;
    }

    /// <summary>
    /// Sets the namespace accessibility to private.
    /// </summary>
    public NamespaceBuilder Private()
    {
        SymbolBuilderExtensions.Private((IAccessible)this);
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateNamespaceSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);
    }
}
