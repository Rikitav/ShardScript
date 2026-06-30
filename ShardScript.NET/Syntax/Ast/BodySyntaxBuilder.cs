using ShardScript.Runtime;

namespace ShardScript.Syntax.Ast;

public sealed class BodySyntaxBuilder
{
    private readonly SyntaxStatementsBlock _block;

    internal BodySyntaxBuilder(SyntaxStatementsBlock block)
    {
        _block = block;
    }

    public BodySyntaxBuilder Return(SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        _block.AddStatement(Syntax.SyntaxBuilder.Return(_block, expression));
        return this;
    }

    public BodySyntaxBuilder Return(Func<ExpressionSyntaxBuilder, SyntaxExpression> expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        return Return(expression(new ExpressionSyntaxBuilder()));
    }

    public BodySyntaxBuilder Declare(string name, PrimitiveType type, SyntaxExpression? initializer = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        SyntaxType syntaxType = CreateType(type);
        _block.AddStatement(Syntax.SyntaxBuilder.Variable(_block, name, syntaxType, initializer));
        return this;
    }

    public BodySyntaxBuilder Assign(SyntaxExpression target, SyntaxExpression value)
    {
        if (target == null)
            throw new ArgumentNullException(nameof(target));
        if (value == null)
            throw new ArgumentNullException(nameof(value));

        _block.AddStatement(Syntax.SyntaxBuilder.ExpressionStatement(_block,
            Syntax.SyntaxBuilder.Binary(target, value, TokenType.AssignOperator)));
        return this;
    }

    private static SyntaxType CreateType(PrimitiveType primitive)
    {
        TokenType token = primitive switch
        {
            PrimitiveType.Boolean => TokenType.BooleanKeyword,
            PrimitiveType.Integer => TokenType.IntegerKeyword,
            PrimitiveType.Double => TokenType.DoubleKeyword,
            PrimitiveType.String => TokenType.StringKeyword,
            PrimitiveType.Char => TokenType.CharKeyword,
            _ => throw new NotSupportedException($"Primitive type '{primitive}' is not supported in AST builder.")
        };

        return Syntax.SyntaxBuilder.PredefinedType(null, token);
    }
}
