using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Nodes;

namespace ShardScript.NET.Syntax.Ast;

public sealed class ClassBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxClassDeclaration _class;

    internal ClassBuilder(CompilationContext context, SyntaxClassDeclaration cls)
    {
        _context = context;
        _class = cls;
    }

    public ClassBuilder Public()
    {
        _class.AddModifier(TokenType.PublicKeyword);
        return this;
    }

    public ClassBuilder Static()
    {
        _class.AddModifier(TokenType.StaticKeyword);
        return this;
    }

    public ClassBuilder AddMethod(string name, Action<MethodBuilder> configure)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxMethodDeclaration method = SyntaxBuilder.Method(_class, name);
        configure(new MethodBuilder(_context, method));
        _class.AddMember(method);
        return this;
    }
}
