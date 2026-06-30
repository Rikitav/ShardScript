using ShardScript.Application;

namespace ShardScript.Syntax.Ast;

public sealed class ClassSyntaxBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxClassDeclaration _class;

    internal ClassSyntaxBuilder(CompilationContext context, SyntaxClassDeclaration cls)
    {
        _context = context;
        _class = cls;
    }

    public ClassSyntaxBuilder Public()
    {
        _class.AddModifier(TokenType.PublicKeyword);
        return this;
    }

    public ClassSyntaxBuilder Static()
    {
        _class.AddModifier(TokenType.StaticKeyword);
        return this;
    }

    public ClassSyntaxBuilder AddMethod(string name, Action<MethodSyntaxBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxMethodDeclaration method = Syntax.SyntaxBuilder.Method(_class, name);
        configure(new MethodSyntaxBuilder(_context, method));
        _class.AddMember(method);
        return this;
    }
}
