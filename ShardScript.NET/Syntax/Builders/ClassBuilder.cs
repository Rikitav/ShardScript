using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Fluent builder for creating class symbols with lambda scoping and automatic type inference.
/// </summary>
public sealed class ClassBuilder : IAccessible, ILinkable, IContainerBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the type symbol being built.
    /// </summary>
    public TypeSymbol Symbol { get; }

    /// <summary>
    /// Gets the parent namespace, if any.
    /// </summary>
    public NamespaceSymbol? ParentNamespace { get; }

    /// <summary>
    /// Gets the name of the class.
    /// </summary>
    public string Name { get; }

    IntPtr IAccessible.Handle => Symbol.Handle;
    IntPtr ILinkable.Handle => Symbol.Handle;
    SyntaxSymbol IContainerBuilder.Symbol => Symbol;

    internal ClassBuilder(CompilationContext context, NamespaceSymbol? parentNamespace, string name)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        Name = name ?? throw new ArgumentNullException(nameof(name));
        ParentNamespace = parentNamespace;

        IntPtr handle = NativeMethods.Shard_CreateClassSymbol(
            context.Handle, parentNamespace?.Handle ?? IntPtr.Zero, name);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create class symbol '{name}'.");

        Symbol = new TypeSymbol(handle);
    }

    /// <summary>
    /// Sets the class accessibility to public.
    /// </summary>
    public ClassBuilder Public()
    {
        SymbolBuilderExtensions.Public((IAccessible)this);
        return this;
    }

    /// <summary>
    /// Sets the class accessibility to private.
    /// </summary>
    public ClassBuilder Private()
    {
        SymbolBuilderExtensions.Private((IAccessible)this);
        return this;
    }

    /// <summary>
    /// Sets the class linking to static.
    /// </summary>
    public ClassBuilder Static()
    {
        SymbolBuilderExtensions.Static((ILinkable)this);
        return this;
    }

    /// <summary>
    /// Sets the class linking to instance.
    /// </summary>
    public ClassBuilder Instance()
    {
        SymbolBuilderExtensions.Instance((ILinkable)this);
        return this;
    }

    /// <summary>
    /// Adds a field with automatic type inference.
    /// </summary>
    public ClassBuilder WithField<T>(string name, Access access = Access.Public)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(typeof(T));
        TypeSymbol fieldType = SymbolBuilder.Primitive(Context, primitive);

        FieldBuilder builder = new FieldBuilder(Context, Symbol, name, fieldType);
        ApplyAccess(builder, access);

        return this;
    }

    /// <summary>
    /// Adds a field with automatic type inference and extracts the field symbol.
    /// </summary>
    public ClassBuilder WithField<T>(string name, Access access, out FieldSymbol field)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(typeof(T));
        TypeSymbol fieldType = SymbolBuilder.Primitive(Context, primitive);

        FieldBuilder builder = new FieldBuilder(Context, Symbol, name, fieldType);
        ApplyAccess(builder, access);
        field = builder.Symbol;

        return this;
    }

    /// <summary>
    /// Adds a field with explicit type symbol.
    /// </summary>
    public ClassBuilder WithField(string name, TypeSymbol type, Access access = Access.Public)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        FieldBuilder builder = new FieldBuilder(Context, Symbol, name, type);
        ApplyAccess(builder, access);

        return this;
    }

    /*
    /// <summary>
    /// Creates a nested class with the specified configuration.
    /// </summary>
    public ClassBuilder WithNestedClass(string name, Action<ClassBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        ClassBuilder nestedBuilder = new ClassBuilder(Context, null, name);
        configure(nestedBuilder);
        return this;
    }

    /// <summary>
    /// Creates a nested class and extracts the symbol.
    /// </summary>
    public ClassBuilder WithNestedClass(string name, Action<ClassBuilder> configure, out TypeSymbol symbol)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        ClassBuilder nestedBuilder = new ClassBuilder(Context, null, name);
        configure(nestedBuilder);
        symbol = nestedBuilder.Symbol;
        return this;
    }
    */

    private static void ApplyAccess(FieldBuilder builder, Access access)
    {
        if (access == Access.Public)
            SymbolBuilderExtensions.Public(builder);
        else
            SymbolBuilderExtensions.Private(builder);
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateClassSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);
    }

    // Methods will be added via generic extensions in SymbolBuilderGenericExtensions.cs
}
