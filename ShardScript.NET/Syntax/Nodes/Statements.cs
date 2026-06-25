namespace ShardScript.Syntax.Nodes;

public sealed class SyntaxVariableStatement : SyntaxStatement
{
    internal SyntaxVariableStatement(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxExpressionStatement : SyntaxStatement
{
    internal SyntaxExpressionStatement(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxReturnStatement : SyntaxStatement
{
    internal SyntaxReturnStatement(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxForEachStatement : SyntaxStatement
{
    internal SyntaxForEachStatement(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxWhileStatement : SyntaxStatement
{
    internal SyntaxWhileStatement(IntPtr handle) : base(handle) { }
}
