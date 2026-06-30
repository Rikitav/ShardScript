using ShardScript.Application;
using ShardScript.Runtime;

namespace ShardScript.Syntax.Ast;

public sealed class MethodSyntaxBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxMethodDeclaration _method;
    private SyntaxParametersList? _parameters;

    internal MethodSyntaxBuilder(CompilationContext context, SyntaxMethodDeclaration method)
    {
        _context = context;
        _method = method;
    }

    public MethodSyntaxBuilder Public()
    {
        _method.AddModifier(TokenType.PublicKeyword);
        return this;
    }

    public MethodSyntaxBuilder Static()
    {
        _method.AddModifier(TokenType.StaticKeyword);
        return this;
    }

    public MethodSyntaxBuilder Parameter(string name, PrimitiveType type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        SyntaxParametersList parameters = _parameters ??= Syntax.SyntaxBuilder.Parameters(_method);
        parameters.AddParameter(name, CreateType(type));
        return this;
    }

    public MethodSyntaxBuilder Returns(PrimitiveType type)
    {
        _method.SetReturnType(CreateType(type));
        return this;
    }

    public MethodSyntaxBuilder Body(Action<BodySyntaxBuilder> configure)
    {
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxStatementsBlock block = Syntax.SyntaxBuilder.Block();
        configure(new BodySyntaxBuilder(block));

        if (_parameters != null)
            _method.SetParameters(_parameters);

        _method.SetBody(block);
        return this;
    }

    private SyntaxType CreateType(PrimitiveType primitive)
    {
        TokenType token = primitive switch
        {
            PrimitiveType.Void => TokenType.VoidKeyword,
            PrimitiveType.Boolean => TokenType.BooleanKeyword,
            PrimitiveType.Integer => TokenType.IntegerKeyword,
            PrimitiveType.Double => TokenType.DoubleKeyword,
            PrimitiveType.String => TokenType.StringKeyword,
            PrimitiveType.Char => TokenType.CharKeyword,
            _ => throw new NotSupportedException($"Primitive type '{primitive}' is not supported in AST builder.")
        };

        return Syntax.SyntaxBuilder.PredefinedType(_method, token);
    }
}
