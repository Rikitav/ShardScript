namespace ShardScript.Syntax.Nodes;

public sealed class SyntaxLiteralExpression : SyntaxExpression
{
    internal SyntaxLiteralExpression(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxIdentifierExpression : SyntaxExpression
{
    internal SyntaxIdentifierExpression(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxMemberAccessExpression : SyntaxExpression
{
    internal SyntaxMemberAccessExpression(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxBinaryExpression : SyntaxExpression
{
    internal SyntaxBinaryExpression(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxUnaryExpression : SyntaxExpression
{
    internal SyntaxUnaryExpression(IntPtr handle) : base(handle) { }
}
