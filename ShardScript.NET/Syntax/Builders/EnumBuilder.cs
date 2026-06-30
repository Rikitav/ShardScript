using ShardScript.Application;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Builder for creating enum symbols programmatically.
/// Note: Programmatic enum creation requires native API support.
/// Currently, enums should be defined in source code.
/// </summary>
public sealed class EnumBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the enum symbol being built.
    /// </summary>
    public EnumSymbol Symbol { get; }

    /// <summary>
    /// Gets the parent namespace.
    /// </summary>
    public NamespaceSymbol? Parent { get; }

    /// <summary>
    /// Gets the name of the enum.
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets whether this is a flags enum.
    /// </summary>
    public EnumKind Kind { get; }

    private readonly List<(string Name, long Value)> _literals = new();

    internal EnumBuilder(CompilationContext context, NamespaceSymbol? parent, string name, EnumKind kind = EnumKind.Standard)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        Name = name ?? throw new ArgumentNullException(nameof(name));
        Parent = parent;
        Kind = kind;

        // Note: Since native enum creation is not available, we create a placeholder
        // In a full implementation, this would call Shard_CreateEnumSymbol
        Symbol = new EnumSymbol(IntPtr.Zero, kind == EnumKind.Flags, 0);
    }

    /// <summary>
    /// Adds a literal to this enum.
    /// </summary>
    public EnumBuilder WithLiteral(string name, long value)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        _literals.Add((name, value));
        return this;
    }

    /// <summary>
    /// Builds the enum symbol.
    /// </summary>
    public EnumSymbol Build()
    {
        // Note: In a full implementation with native support, this would:
        // 1. Call Shard_CreateEnumSymbol if not already created
        // 2. Call Shard_AddEnumLiteral for each literal
        // 3. Return the completed symbol

        return Symbol;
    }
}
