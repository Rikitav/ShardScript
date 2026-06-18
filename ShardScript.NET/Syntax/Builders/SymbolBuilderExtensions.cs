using ShardScript.NET.Syntax.Symbols;

namespace ShardScript.NET.Syntax.Builders;

public static class SymbolBuilderExtensions
{
    public static T Public<T>(this ISymbolBuilder<T> builder) where T : SyntaxSymbol, IAccessible<T>
    {
        _accessibility = SymbolAccessibility.Public;
        return this;
    }

    public FieldBuilder Private()
    {
        _accessibility = SymbolAccessibility.Private;
        return this;
    }

    public FieldBuilder Static()
    {
        _linking = SymbolLinking.Static;
        return this;
    }

    public FieldBuilder Instance()
    {
        _linking = SymbolLinking.Instance;
        return this;
    }

    private static class NativeMethods
    {

    }
}
