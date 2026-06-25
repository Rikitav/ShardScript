using ShardScript.Application;
using ShardScript.Syntax.Nodes;

namespace ShardScript.Syntax.Ast;

/// <summary>
/// Entry point for building ShardScript syntax trees using a scoped, fluent API.
/// </summary>
public static class AstBuilder
{
    public static SyntaxCompilationUnit Unit(CompilationContext context, Action<CompilationUnitBuilder> configure)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxCompilationUnit unit = SyntaxBuilder.Unit(context).SetOrigin(CompilationUnitOrigin.SourceFile);
        configure(new CompilationUnitBuilder(context, unit));
        return unit;
    }
}
