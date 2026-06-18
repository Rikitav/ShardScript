using ShardScript.NET.Syntax.Symbols;

namespace ShardScript.NET.Syntax.Builders;

public interface ISymbolBuilder<out TSymbol> where TSymbol : SyntaxSymbol
{
    IntPtr Handle { get; }
    TSymbol Symbol { get; }
}

public interface IAccessible<out TBuilder> where TBuilder : IAccessible<TBuilder>
{
    IntPtr Handle { get; }
}

public interface ILinkable<out TBuilder> where TBuilder : ILinkable<TBuilder>
{
    IntPtr Handle { get; }
}

public interface IParameterizable<out TBuilder> where TBuilder : IParameterizable<TBuilder>
{
    IntPtr Handle { get; }
}

public interface ICallback<out TBuilder> where TBuilder : ICallback<TBuilder>
{
    IntPtr Handle { get; }
}
