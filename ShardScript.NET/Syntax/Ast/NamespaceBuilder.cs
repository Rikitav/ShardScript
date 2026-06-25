using ShardScript.Application;

namespace ShardScript.Syntax.Ast;

public sealed class NamespaceBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxCompilationUnit _unit;
    private readonly SyntaxNamespaceDeclaration _ns;

    internal NamespaceBuilder(CompilationContext context, SyntaxCompilationUnit unit, SyntaxNamespaceDeclaration ns)
    {
        _context = context;
        _unit = unit;
        _ns = ns;
    }

    public NamespaceBuilder AddClass(string name, Action<ClassBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxClassDeclaration cls = SyntaxBuilder.Class(_ns, name);
        configure(new ClassBuilder(_context, cls));
        _unit.AddMember(cls);
        return this;
    }
}
