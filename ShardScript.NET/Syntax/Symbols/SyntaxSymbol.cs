namespace ShardScript.NET.Syntax.Symbols;

public enum SymbolAccessibility
{
    Private = 0,
    Public = 1,
    Protected = 2
}

public enum SymbolLinking
{
    Static = 0,
    Instance = 1
}

public abstract class SyntaxSymbol(IntPtr handle)
{
    public IntPtr Handle { get; } = handle;
}
