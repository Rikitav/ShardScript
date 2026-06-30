using ShardScript.Application;

namespace ShardScript.Syntax.Ast;

public sealed class CompilationUnitSyntaxBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxCompilationUnit _unit;

    internal CompilationUnitSyntaxBuilder(CompilationContext context, SyntaxCompilationUnit unit)
    {
        _context = context;
        _unit = unit;
    }

    public CompilationUnitSyntaxBuilder InNamespace(string name, Action<NamespaceSyntaxBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxNamespaceDeclaration ns = Syntax.SyntaxBuilder.Namespace().AddIdentifier(name);
        _unit.SetNamespace(ns);
        configure(new NamespaceSyntaxBuilder(_context, _unit, ns));
        return this;
    }

    public CompilationUnitSyntaxBuilder AddClass(string name, Action<ClassSyntaxBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxClassDeclaration cls = Syntax.SyntaxBuilder.Class(_unit, name);
        configure(new ClassSyntaxBuilder(_context, cls));
        _unit.AddMember(cls);
        return this;
    }
}
