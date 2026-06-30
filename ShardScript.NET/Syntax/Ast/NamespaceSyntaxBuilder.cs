using ShardScript.Application;

namespace ShardScript.Syntax.Ast;

public sealed class NamespaceSyntaxBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxCompilationUnit _unit;
    private readonly SyntaxNamespaceDeclaration _ns;

    internal NamespaceSyntaxBuilder(CompilationContext context, SyntaxCompilationUnit unit, SyntaxNamespaceDeclaration ns)
    {
        _context = context;
        _unit = unit;
        _ns = ns;
    }

    public NamespaceSyntaxBuilder AddClass(string name, Action<ClassSyntaxBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxClassDeclaration cls = Syntax.SyntaxBuilder.Class(_ns, name);
        configure(new ClassSyntaxBuilder(_context, cls));
        _unit.AddMember(cls);
        return this;
    }
}
