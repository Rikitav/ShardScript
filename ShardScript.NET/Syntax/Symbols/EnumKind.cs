namespace ShardScript.Syntax.Symbols;

/// <summary>
/// Specifies the kind of enum declaration.
/// </summary>
public enum EnumKind
{
    /// <summary>
    /// Standard enum with mutually exclusive values.
    /// </summary>
    Standard,

    /// <summary>
    /// Flags enum that supports bitwise operations.
    /// </summary>
    Flags
}
