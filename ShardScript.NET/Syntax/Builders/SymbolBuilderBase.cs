using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;

namespace ShardScript.NET.Syntax.Builders;

public interface ISymbolBuilder<out TSymbol> where TSymbol : SyntaxSymbol
{
    CompilationContext Context { get; }
    IntPtr Handle { get; }
    TSymbol Symbol { get; }
}

public interface IAccessible
{
    CompilationContext Context { get; }
    IntPtr Handle { get; }
}

public interface ILinkable
{
    CompilationContext Context { get; }
    IntPtr Handle { get; }
}

public interface IParameterizable
{
    CompilationContext Context { get; }
    IntPtr Handle { get; }
}

public interface ICallback
{
    CompilationContext Context { get; }
    IntPtr Handle { get; }

    void KeepCallbackAlive(ShardManagedMethodCallbackNative callback);
}
