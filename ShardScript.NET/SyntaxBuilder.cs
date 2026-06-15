using System.Runtime.InteropServices;

namespace ShardScript.NET;

public enum TokenType
{
    Unknown = 0,
    EndOfFile = 1,
    Trivia = 2,
    NewKeyword = 3,
    Identifier = 4,
    AssignOperator = 5,
    DeclareAssignOperator = 6,
    ArrowOperator = 7,
    LambdaOperator = 8,
    NullCoalescingOperator = 9,
    RangeOperator = 10,
    RangeInclusiveOperator = 11,
    AddOperator = 12,
    SubOperator = 13,
    MultOperator = 14,
    DivOperator = 15,
    ModOperator = 16,
    PowOperator = 17,
    AddAssignOperator = 18,
    SubAssignOperator = 19,
    MultAssignOperator = 20,
    DivAssignOperator = 21,
    ModAssignOperator = 22,
    PowAssignOperator = 23,
    IncrementOperator = 24,
    DecrementOperator = 25,
    OrOperator = 26,
    AndOperator = 27,
    RightShiftOperator = 28,
    LeftShiftOperator = 29,
    OrAssignOperator = 30,
    AndAssignOperator = 31,
    EqualsOperator = 32,
    NotEqualsOperator = 33,
    GreaterOperator = 34,
    GreaterOrEqualsOperator = 35,
    LessOperator = 36,
    LessOrEqualsOperator = 37,
    NotOperator = 38,
    NullLiteral = 39,
    CharLiteral = 40,
    StringLiteral = 41,
    BooleanLiteral = 42,
    NumberLiteral = 43,
    DoubleLiteral = 44,
    NativeLiteral = 45,
    Colon = 46,
    Semicolon = 47,
    OpenBrace = 48,
    CloseBrace = 49,
    OpenCurl = 50,
    CloseCurl = 51,
    OpenSquare = 52,
    CloseSquare = 53,
    Delimeter = 54,
    Comma = 55,
    Question = 56,
    PublicKeyword = 57,
    PrivateKeyword = 58,
    ProtectedKeyword = 59,
    InternalKeyword = 60,
    StaticKeyword = 61,
    ExternKeyword = 62,
    ExportKeyword = 63,
    GetKeyword = 64,
    SetKeyword = 65,
    FieldKeyword = 66,
    IndexerKeyword = 67,
    VoidKeyword = 68,
    VarKeyword = 69,
    IntegerKeyword = 70,
    DoubleKeyword = 71,
    ShortKeyword = 72,
    LongKeyword = 73,
    CharKeyword = 74,
    StringKeyword = 75,
    BooleanKeyword = 76,
    DelegateKeyword = 77,
    LambdaKeyword = 78,
    UsingKeyword = 79,
    FromKeyword = 80,
    ImportKeyword = 81,
    ConstKeyword = 82,
    ValueKeyword = 83,
    MethodKeyword = 84,
    ClassKeyword = 85,
    StructKeyword = 86,
    InterfaceKeyword = 87,
    NamespaceKeyword = 88,
    FunctionKeyword = 89,
    InitKeyword = 90,
    ForKeyword = 91,
    WhileKeyword = 92,
    UntilKeyword = 93,
    DoKeyword = 94,
    ForeachKeyword = 95,
    InKeyword = 96,
    IfKeyword = 97,
    UnlessKeyword = 98,
    ElseKeyword = 99,
    SwitchKeyword = 100,
    ReturnKeyword = 101,
    ThrowKeyword = 102,
    BreakKeyword = 103,
    ContinueKeyword = 104,
    TryKeyword = 105,
    CatchKeyword = 106
}

public abstract class SyntaxNode
{
    public IntPtr Handle { get; }

    internal SyntaxNode(IntPtr handle)
    {
        Handle = handle;
    }

    internal static IntPtr CreateHandle(Func<IntPtr> factory)
    {
        IntPtr handle = factory();
        if (handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create syntax node: {ShardException.GetLastError()}");

        return handle;
    }

    internal static void ThrowIfError(int result)
    {
        if (result != 0)
            throw new InvalidOperationException($"Failed to modify syntax node: {ShardException.GetLastError()}");
    }

    internal static IntPtr ToNative(SyntaxNode? node) => node?.Handle ?? IntPtr.Zero;
}

public abstract class SyntaxMemberDeclaration : SyntaxNode
{
    internal SyntaxMemberDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxMemberDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public abstract class SyntaxTypeDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxTypeDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxTypeDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(Native.Shard_AddTypeMember(Handle, member.Handle));
        return this;
    }
}

public abstract class SyntaxStatement : SyntaxNode
{
    internal SyntaxStatement(IntPtr handle) : base(handle) { }
}

public abstract class SyntaxExpression : SyntaxNode
{
    internal SyntaxExpression(IntPtr handle) : base(handle) { }
}

public abstract class SyntaxType : SyntaxNode
{
    internal SyntaxType(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxCompilationUnit : SyntaxNode
{
    internal SyntaxCompilationUnit(IntPtr handle) : base(handle) { }

    public SyntaxCompilationUnit SetNamespace(SyntaxNamespaceDeclaration ns)
    {
        if (ns == null)
            throw new ArgumentNullException(nameof(ns));

        ThrowIfError(Native.Shard_SetCompilationUnitNamespace(Handle, ns.Handle));
        return this;
    }

    public SyntaxCompilationUnit SetOrigin(CompilationUnitOrigin origin)
    {
        ThrowIfError(Native.Shard_SetCompilationUnitOrigin(Handle, (int)origin));
        return this;
    }

    public SyntaxCompilationUnit AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(Native.Shard_AddCompilationUnitMember(Handle, member.Handle));
        return this;
    }
}

public sealed class SyntaxNamespaceDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxNamespaceDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxNamespaceDeclaration AddIdentifier(string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        ThrowIfError(Native.Shard_AddNamespaceIdentifier(Handle, name));
        return this;
    }

    new public SyntaxNamespaceDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxClassDeclaration : SyntaxTypeDeclaration
{
    internal SyntaxClassDeclaration(IntPtr handle) : base(handle) { }

    new public SyntaxClassDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(Native.Shard_AddTypeMember(Handle, member.Handle));
        return this;
    }

    new public SyntaxClassDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxStructDeclaration : SyntaxTypeDeclaration
{
    internal SyntaxStructDeclaration(IntPtr handle) : base(handle) { }

    new public SyntaxStructDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(Native.Shard_AddTypeMember(Handle, member.Handle));
        return this;
    }

    new public SyntaxStructDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxFieldDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxFieldDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxFieldDeclaration SetInitializer(SyntaxExpression? expression)
    {
        ThrowIfError(Native.Shard_SetFieldInitializer(Handle, ToNative(expression)));
        return this;
    }

    new public SyntaxFieldDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxMethodDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxMethodDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxMethodDeclaration SetReturnType(SyntaxType? returnType)
    {
        ThrowIfError(Native.Shard_SetMethodReturnType(Handle, ToNative(returnType)));
        return this;
    }

    public SyntaxMethodDeclaration SetParameters(SyntaxParametersList parameters)
    {
        if (parameters == null)
            throw new ArgumentNullException(nameof(parameters));

        ThrowIfError(Native.Shard_SetMethodParametersList(Handle, parameters.Handle));
        return this;
    }

    public SyntaxMethodDeclaration SetBody(SyntaxStatementsBlock body)
    {
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        ThrowIfError(Native.Shard_SetMethodBody(Handle, body.Handle));
        return this;
    }

    new public SyntaxMethodDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxConstructorDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxConstructorDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxConstructorDeclaration SetParameters(SyntaxParametersList parameters)
    {
        if (parameters == null)
            throw new ArgumentNullException(nameof(parameters));

        ThrowIfError(Native.Shard_SetConstructorParametersList(Handle, parameters.Handle));
        return this;
    }

    public SyntaxConstructorDeclaration SetBody(SyntaxStatementsBlock body)
    {
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        ThrowIfError(Native.Shard_SetConstructorBody(Handle, body.Handle));
        return this;
    }

    new public SyntaxConstructorDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(Native.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }
}

public sealed class SyntaxParametersList : SyntaxNode
{
    internal SyntaxParametersList(IntPtr handle) : base(handle) { }

    public SyntaxParametersList AddParameter(string name, SyntaxType type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        ThrowIfError(Native.Shard_AddParameter(Handle, name, type.Handle));
        return this;
    }
}

public sealed class SyntaxStatementsBlock : SyntaxNode
{
    internal SyntaxStatementsBlock(IntPtr handle) : base(handle) { }

    public SyntaxStatementsBlock AddStatement(SyntaxStatement statement)
    {
        if (statement == null)
            throw new ArgumentNullException(nameof(statement));

        ThrowIfError(Native.Shard_AddStatement(Handle, statement.Handle));
        return this;
    }
}

public sealed class SyntaxPredefinedType : SyntaxType
{
    internal SyntaxPredefinedType(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxIdentifierNameType : SyntaxType
{
    internal SyntaxIdentifierNameType(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxArrayType : SyntaxType
{
    internal SyntaxArrayType(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxNullableType : SyntaxType
{
    internal SyntaxNullableType(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxGenericType : SyntaxType
{
    internal SyntaxGenericType(IntPtr handle) : base(handle) { }

    public SyntaxGenericType SetArguments(SyntaxTypeArgumentsList arguments)
    {
        if (arguments == null)
            throw new ArgumentNullException(nameof(arguments));

        ThrowIfError(Native.Shard_SetGenericTypeArguments(Handle, arguments.Handle));
        return this;
    }
}

public sealed class SyntaxTypeArgumentsList : SyntaxNode
{
    internal SyntaxTypeArgumentsList(IntPtr handle) : base(handle) { }

    public SyntaxTypeArgumentsList AddArgument(SyntaxType type)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        ThrowIfError(Native.Shard_AddTypeArgument(Handle, type.Handle));
        return this;
    }
}

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

public sealed class SyntaxInvocationExpression : SyntaxExpression
{
    internal SyntaxInvocationExpression(IntPtr handle) : base(handle) { }

    public SyntaxInvocationExpression SetArguments(SyntaxArgumentsList arguments)
    {
        if (arguments == null)
            throw new ArgumentNullException(nameof(arguments));

        ThrowIfError(Native.Shard_SetInvocationArgumentsList(Handle, arguments.Handle));
        return this;
    }
}

public sealed class SyntaxObjectExpression : SyntaxExpression
{
    internal SyntaxObjectExpression(IntPtr handle) : base(handle) { }

    public SyntaxObjectExpression SetArguments(SyntaxArgumentsList arguments)
    {
        if (arguments == null)
            throw new ArgumentNullException(nameof(arguments));

        ThrowIfError(Native.Shard_SetObjectArgumentsList(Handle, arguments.Handle));
        return this;
    }
}

public sealed class SyntaxRangeExpression : SyntaxExpression
{
    internal SyntaxRangeExpression(IntPtr handle) : base(handle) { }
}

public sealed class SyntaxCollectionExpression : SyntaxExpression
{
    internal SyntaxCollectionExpression(IntPtr handle) : base(handle) { }

    public SyntaxCollectionExpression AddElement(SyntaxExpression element)
    {
        if (element == null)
            throw new ArgumentNullException(nameof(element));

        ThrowIfError(Native.Shard_AddCollectionElement(Handle, element.Handle));
        return this;
    }
}

public sealed class SyntaxArgumentsList : SyntaxNode
{
    internal SyntaxArgumentsList(IntPtr handle) : base(handle) { }

    public SyntaxArgumentsList AddArgument(SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        ThrowIfError(Native.Shard_AddArgument(Handle, expression.Handle));
        return this;
    }
}

public static class SyntaxBuilder
{
    public static SyntaxCompilationUnit Unit(CompilationContext context)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        return new SyntaxCompilationUnit(SyntaxNode.CreateHandle(() => Native.Shard_CreateCompilationUnit(context.Handle)));
    }

    public static SyntaxNamespaceDeclaration Namespace(SyntaxNode? parent = null)
        => new SyntaxNamespaceDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateNamespaceDeclaration(SyntaxNode.ToNative(parent))));

    public static SyntaxClassDeclaration Class(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxClassDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateClassDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxStructDeclaration Struct(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxStructDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateStructDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxFieldDeclaration Field(SyntaxNode? parent, string name, SyntaxType type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SyntaxFieldDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateFieldDeclaration(SyntaxNode.ToNative(parent), name, type.Handle)));
    }

    public static SyntaxMethodDeclaration Method(SyntaxNode? parent, string name, SyntaxType? returnType = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxMethodDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateMethodDeclaration(SyntaxNode.ToNative(parent), name, SyntaxNode.ToNative(returnType))));
    }

    public static SyntaxConstructorDeclaration Constructor(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxConstructorDeclaration(SyntaxNode.CreateHandle(() => Native.Shard_CreateConstructorDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxParametersList Parameters(SyntaxNode? parent = null)
        => new SyntaxParametersList(SyntaxNode.CreateHandle(() => Native.Shard_CreateParametersList(SyntaxNode.ToNative(parent))));

    public static SyntaxStatementsBlock Block(SyntaxNode? parent = null)
        => new SyntaxStatementsBlock(SyntaxNode.CreateHandle(() => Native.Shard_CreateStatementsBlock(SyntaxNode.ToNative(parent))));

    public static SyntaxArgumentsList Arguments(SyntaxNode? parent = null)
        => new SyntaxArgumentsList(SyntaxNode.CreateHandle(() => Native.Shard_CreateArgumentsList(SyntaxNode.ToNative(parent))));

    public static SyntaxTypeArgumentsList TypeArguments(SyntaxNode? parent = null)
        => new SyntaxTypeArgumentsList(SyntaxNode.CreateHandle(() => Native.Shard_CreateTypeArgumentsList(SyntaxNode.ToNative(parent))));

    public static SyntaxPredefinedType PredefinedType(SyntaxNode? parent, TokenType tokenType)
        => new SyntaxPredefinedType(SyntaxNode.CreateHandle(() => Native.Shard_CreatePredefinedType(SyntaxNode.ToNative(parent), (int)tokenType)));

    public static SyntaxIdentifierNameType IdentifierType(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxIdentifierNameType(SyntaxNode.CreateHandle(() => Native.Shard_CreateIdentifierNameType(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxArrayType ArrayType(SyntaxNode? parent, SyntaxType elementType, int rank = 1)
    {
        if (elementType == null)
            throw new ArgumentNullException(nameof(elementType));

        return new SyntaxArrayType(SyntaxNode.CreateHandle(() => Native.Shard_CreateArrayType(SyntaxNode.ToNative(parent), elementType.Handle, rank)));
    }

    public static SyntaxNullableType NullableType(SyntaxNode? parent, SyntaxType underlyingType)
    {
        if (underlyingType == null)
            throw new ArgumentNullException(nameof(underlyingType));

        return new SyntaxNullableType(SyntaxNode.CreateHandle(() => Native.Shard_CreateNullableType(SyntaxNode.ToNative(parent), underlyingType.Handle)));
    }

    public static SyntaxGenericType GenericType(SyntaxNode? parent, SyntaxType underlyingType)
    {
        if (underlyingType == null)
            throw new ArgumentNullException(nameof(underlyingType));

        return new SyntaxGenericType(SyntaxNode.CreateHandle(() => Native.Shard_CreateGenericType(SyntaxNode.ToNative(parent), underlyingType.Handle)));
    }

    public static SyntaxPredefinedType VoidType(SyntaxNode? parent = null) => PredefinedType(parent, TokenType.VoidKeyword);
    public static SyntaxPredefinedType IntType(SyntaxNode? parent = null) => PredefinedType(parent, TokenType.IntegerKeyword);
    public static SyntaxPredefinedType DoubleType(SyntaxNode? parent = null) => PredefinedType(parent, TokenType.DoubleKeyword);
    public static SyntaxPredefinedType BoolType(SyntaxNode? parent = null) => PredefinedType(parent, TokenType.BooleanKeyword);
    public static SyntaxPredefinedType StringType(SyntaxNode? parent = null) => PredefinedType(parent, TokenType.StringKeyword);

    public static SyntaxVariableStatement Variable(SyntaxNode? parent, string name, SyntaxType? type = null, SyntaxExpression? initializer = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxVariableStatement(SyntaxNode.CreateHandle(() => Native.Shard_CreateVariableStatement(SyntaxNode.ToNative(parent), name, SyntaxNode.ToNative(type), SyntaxNode.ToNative(initializer))));
    }

    public static SyntaxExpressionStatement ExpressionStatement(SyntaxNode? parent, SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        return new SyntaxExpressionStatement(SyntaxNode.CreateHandle(() => Native.Shard_CreateExpressionStatement(SyntaxNode.ToNative(parent), expression.Handle)));
    }

    public static SyntaxReturnStatement Return(SyntaxNode? parent, SyntaxExpression? expression = null)
        => new SyntaxReturnStatement(SyntaxNode.CreateHandle(() => Native.Shard_CreateReturnStatement(SyntaxNode.ToNative(parent), SyntaxNode.ToNative(expression))));

    public static SyntaxForEachStatement ForEach(SyntaxNode? parent, string variableName, SyntaxExpression range, SyntaxStatementsBlock body)
    {
        if (variableName == null)
            throw new ArgumentNullException(nameof(variableName));
        if (range == null)
            throw new ArgumentNullException(nameof(range));
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        return new SyntaxForEachStatement(SyntaxNode.CreateHandle(() => Native.Shard_CreateForEachStatement(SyntaxNode.ToNative(parent), variableName, range.Handle, body.Handle)));
    }

    public static SyntaxWhileStatement While(SyntaxNode? parent, SyntaxExpression condition, SyntaxStatementsBlock body)
    {
        if (condition == null)
            throw new ArgumentNullException(nameof(condition));
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        return new SyntaxWhileStatement(SyntaxNode.CreateHandle(() => Native.Shard_CreateWhileStatement(SyntaxNode.ToNative(parent), condition.Handle, body.Handle)));
    }

    public static SyntaxLiteralExpression Literal(TokenType tokenType, string value, SyntaxNode? parent = null)
    {
        if (value == null)
            throw new ArgumentNullException(nameof(value));

        return new SyntaxLiteralExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateLiteralExpression(SyntaxNode.ToNative(parent), (int)tokenType, value)));
    }

    public static SyntaxLiteralExpression Literal(int value, SyntaxNode? parent = null)
        => Literal(TokenType.NumberLiteral, value.ToString(), parent);

    public static SyntaxLiteralExpression Literal(double value, SyntaxNode? parent = null)
        => Literal(TokenType.DoubleLiteral, value.ToString(System.Globalization.CultureInfo.InvariantCulture), parent);

    public static SyntaxLiteralExpression Literal(bool value, SyntaxNode? parent = null)
        => Literal(TokenType.BooleanLiteral, value ? "true" : "false", parent);

    public static SyntaxLiteralExpression Literal(string value, SyntaxNode? parent = null)
        => Literal(TokenType.StringLiteral, value, parent);

    public static SyntaxIdentifierExpression Identifier(string name, SyntaxNode? parent = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxIdentifierExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateIdentifierExpression(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxMemberAccessExpression MemberAccess(SyntaxExpression? previous, string memberName, SyntaxNode? parent = null)
    {
        if (memberName == null)
            throw new ArgumentNullException(nameof(memberName));

        return new SyntaxMemberAccessExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateMemberAccessExpression(SyntaxNode.ToNative(parent), SyntaxNode.ToNative(previous), memberName)));
    }

    public static SyntaxBinaryExpression Binary(SyntaxExpression left, SyntaxExpression right, TokenType operatorTokenType, SyntaxNode? parent = null)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));

        return new SyntaxBinaryExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateBinaryExpression(SyntaxNode.ToNative(parent), left.Handle, right.Handle, (int)operatorTokenType)));
    }

    public static SyntaxUnaryExpression Unary(SyntaxExpression operand, TokenType operatorTokenType, bool postfix = false, SyntaxNode? parent = null)
    {
        if (operand == null)
            throw new ArgumentNullException(nameof(operand));

        return new SyntaxUnaryExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateUnaryExpression(SyntaxNode.ToNative(parent), operand.Handle, (int)operatorTokenType, postfix ? 1 : 0)));
    }

    public static SyntaxInvocationExpression Invocation(SyntaxExpression target, string? methodName = null, SyntaxNode? parent = null)
    {
        if (target == null)
            throw new ArgumentNullException(nameof(target));

        return new SyntaxInvocationExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateInvocationExpression(SyntaxNode.ToNative(parent), target.Handle, methodName)));
    }

    public static SyntaxObjectExpression Object(SyntaxType type, SyntaxNode? parent = null)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SyntaxObjectExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateObjectExpression(SyntaxNode.ToNative(parent), type.Handle)));
    }

    public static SyntaxRangeExpression Range(SyntaxExpression left, SyntaxExpression right, bool inclusive = false, SyntaxNode? parent = null)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));

        return new SyntaxRangeExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateRangeExpression(SyntaxNode.ToNative(parent), left.Handle, right.Handle, inclusive ? 1 : 0)));
    }

    public static SyntaxCollectionExpression Collection(SyntaxNode? parent = null)
        => new SyntaxCollectionExpression(SyntaxNode.CreateHandle(() => Native.Shard_CreateCollectionExpression(SyntaxNode.ToNative(parent))));
}
