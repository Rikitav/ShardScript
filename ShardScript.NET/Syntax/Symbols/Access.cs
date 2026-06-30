namespace ShardScript.Syntax.Symbols;

/// <summary>
/// Specifies the accessibility level of a symbol.
/// </summary>
public enum Access
{
    /// <summary>
    /// The symbol is publicly accessible.
    /// </summary>
    Public,

    /// <summary>
    /// The symbol is only accessible within its containing type.
    /// </summary>
    Private
}
