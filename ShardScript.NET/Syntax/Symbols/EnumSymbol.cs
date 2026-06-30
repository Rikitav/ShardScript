using ShardScript.Application;

namespace ShardScript.Syntax.Symbols;

/// <summary>
/// Represents an enum symbol in ShardScript.
/// Note: Programmatic enum creation is not currently supported via native API.
/// Use source code compilation for enum declarations.
/// </summary>
public sealed class EnumSymbol : SyntaxSymbol
{
    /// <summary>
    /// Gets whether this is a flags enum.
    /// </summary>
    public bool IsFlags { get; }

    /// <summary>
    /// Gets the number of literals defined in this enum.
    /// </summary>
    public int LiteralCount { get; }

    internal EnumSymbol(IntPtr handle, bool isFlags = false, int literalCount = 0)
        : base(handle)
    {
        IsFlags = isFlags;
        LiteralCount = literalCount;
    }
}
