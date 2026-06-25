using ShardScript.Application;

namespace ShardScript.Syntax.Ast;

public sealed class CompilationUnitBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxCompilationUnit _unit;

    internal CompilationUnitBuilder(CompilationContext context, SyntaxCompilationUnit unit)
    {
        _context = context;
        _unit = unit;
    }

    public CompilationUnitBuilder InNamespace(string name, Action<NamespaceBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxNamespaceDeclaration ns = SyntaxBuilder.Namespace().AddIdentifier(name);
        _unit.SetNamespace(ns);
        configure(new NamespaceBuilder(_context, _unit, ns));
        return this;
    }

    public CompilationUnitBuilder AddClass(string name, Action<ClassBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxClassDeclaration cls = SyntaxBuilder.Class(_unit, name);
        configure(new ClassBuilder(_context, cls));
        _unit.AddMember(cls);
        return this;
    }
}
