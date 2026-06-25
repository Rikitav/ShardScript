using ShardScript.Runtime;

namespace ShardScript.Syntax.Ast;

public sealed class BodyBuilder
{
    private readonly SyntaxStatementsBlock _block;

    internal BodyBuilder(SyntaxStatementsBlock block)
    {
        _block = block;
    }

    public BodyBuilder Return(SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        _block.AddStatement(SyntaxBuilder.Return(_block, expression));
        return this;
    }

    public BodyBuilder Return(Func<ExpressionBuilder, SyntaxExpression> expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        return Return(expression(new ExpressionBuilder()));
    }

    public BodyBuilder Declare(string name, PrimitiveType type, SyntaxExpression? initializer = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        SyntaxType syntaxType = CreateType(type);
        _block.AddStatement(SyntaxBuilder.Variable(_block, name, syntaxType, initializer));
        return this;
    }

    public BodyBuilder Assign(SyntaxExpression target, SyntaxExpression value)
    {
        if (target == null)
            throw new ArgumentNullException(nameof(target));
        if (value == null)
            throw new ArgumentNullException(nameof(value));

        _block.AddStatement(SyntaxBuilder.ExpressionStatement(_block,
            SyntaxBuilder.Binary(target, value, TokenType.AssignOperator)));
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

        return SyntaxBuilder.PredefinedType(null, token);
    }
}
