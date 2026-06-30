using ShardScript.Application;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

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

/// <summary>
/// Marker for fluent builders that can contain members (namespaces and types).
/// Enables shared <c>WithMethod</c> overloads that attach methods to either a
/// namespace (free functions) or a type.
/// </summary>
public interface IContainerBuilder
{
    /// <summary>Compilation context owning the symbols.</summary>
    CompilationContext Context { get; }

    /// <summary>The container symbol (namespace or type) members attach to.</summary>
    SyntaxSymbol Symbol { get; }
}
