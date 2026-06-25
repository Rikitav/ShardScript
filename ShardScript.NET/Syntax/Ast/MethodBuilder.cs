using ShardScript.NET.Application;
using ShardScript.NET.Runtime;
using ShardScript.NET.Syntax.Nodes;

namespace ShardScript.NET.Syntax.Ast;

public sealed class MethodBuilder
{
    private readonly CompilationContext _context;
    private readonly SyntaxMethodDeclaration _method;
    private SyntaxParametersList? _parameters;

    internal MethodBuilder(CompilationContext context, SyntaxMethodDeclaration method)
    {
        _context = context;
        _method = method;
    }

    public MethodBuilder Public()
    {
        _method.AddModifier(TokenType.PublicKeyword);
        return this;
    }

    public MethodBuilder Static()
    {
        _method.AddModifier(TokenType.StaticKeyword);
        return this;
    }

    public MethodBuilder Parameter(string name, PrimitiveType type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        SyntaxParametersList parameters = _parameters ??= SyntaxBuilder.Parameters(_method);
        parameters.AddParameter(name, CreateType(type));
        return this;
    }

    public MethodBuilder Returns(PrimitiveType type)
    {
        _method.SetReturnType(CreateType(type));
        return this;
    }

    public MethodBuilder Body(Action<BodyBuilder> configure)
    {
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxStatementsBlock block = SyntaxBuilder.Block();
        configure(new BodyBuilder(block));

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

        return SyntaxBuilder.PredefinedType(_method, token);
    }
}
