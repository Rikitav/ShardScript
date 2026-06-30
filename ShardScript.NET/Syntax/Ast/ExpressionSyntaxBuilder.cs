using ShardScript.Syntax.Nodes;

namespace ShardScript.Syntax.Ast;

/// <summary>
/// Factory for ShardScript expressions. Supports operator overloading on <see cref="SyntaxExpression"/>.
/// </summary>
public sealed class ExpressionSyntaxBuilder
{
    public SyntaxIdentifierExpression Identifier(string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return Syntax.SyntaxBuilder.Identifier(name);
    }

    public SyntaxLiteralExpression Literal(long value) => Syntax.SyntaxBuilder.Literal(value);
    public SyntaxLiteralExpression Literal(double value) => Syntax.SyntaxBuilder.Literal(value);
    public SyntaxLiteralExpression Literal(bool value) => Syntax.SyntaxBuilder.Literal(value);
    public SyntaxLiteralExpression Literal(string value)
    {
        if (value == null)
            throw new ArgumentNullException(nameof(value));

        return Syntax.SyntaxBuilder.Literal(value);
    }

    public SyntaxMemberAccessExpression MemberAccess(SyntaxExpression previous, string memberName)
    {
        if (previous == null)
            throw new ArgumentNullException(nameof(previous));
        if (memberName == null)
            throw new ArgumentNullException(nameof(memberName));

        return Syntax.SyntaxBuilder.MemberAccess(previous, memberName);
    }

    public SyntaxInvocationExpression Call(SyntaxExpression target, params SyntaxExpression[] arguments)
    {
        if (target == null)
            throw new ArgumentNullException(nameof(target));

        SyntaxInvocationExpression invocation = Syntax.SyntaxBuilder.Invocation(target);
        if (arguments.Length > 0)
        {
            SyntaxArgumentsList list = Syntax.SyntaxBuilder.Arguments();
            foreach (SyntaxExpression arg in arguments)
                list.AddArgument(arg);

            invocation.SetArguments(list);
        }

        return invocation;
    }
}
