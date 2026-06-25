using ShardScript.Application;
using ShardScript.Syntax.Nodes;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax;

public enum TokenType
{
    Unknown,
    EndOfFile,
    Trivia,
    NewKeyword,
    Identifier,
    AssignOperator,
    DeclareAssignOperator,
    ArrowOperator,
    LambdaOperator,
    NullCoalescingOperator,
    RangeOperator,
    RangeInclusiveOperator,
    AddOperator,
    SubOperator,
    MultOperator,
    DivOperator,
    ModOperator,
    PowOperator,
    AddAssignOperator,
    SubAssignOperator,
    MultAssignOperator,
    DivAssignOperator,
    ModAssignOperator,
    PowAssignOperator,
    IncrementOperator,
    DecrementOperator,
    OrOperator,
    AndOperator,
    RightShiftOperator,
    LeftShiftOperator,
    OrAssignOperator,
    AndAssignOperator,
    EqualsOperator,
    NotEqualsOperator,
    GreaterOperator,
    GreaterOrEqualsOperator,
    LessOperator,
    LessOrEqualsOperator,
    NotOperator,
    IsOperator,
    AsOperator,
    NullLiteral,
    CharLiteral,
    StringLiteral,
    BooleanLiteral,
    NumberLiteral,
    DoubleLiteral,
    NativeLiteral,
    Colon,
    Semicolon,
    OpenBrace,
    CloseBrace,
    OpenCurl,
    CloseCurl,
    OpenSquare,
    CloseSquare,
    Delimeter,
    Comma,
    Question,
    PublicKeyword,
    PrivateKeyword,
    ProtectedKeyword,
    InternalKeyword,
    StaticKeyword,
    ExternKeyword,
    ExportKeyword,
    GetKeyword,
    SetKeyword,
    FieldKeyword,
    IndexerKeyword,
    VoidKeyword,
    VarKeyword,
    IntegerKeyword,
    DoubleKeyword,
    ShortKeyword,
    LongKeyword,
    CharKeyword,
    StringKeyword,
    BooleanKeyword,
    DelegateKeyword,
    LambdaKeyword,
    UsingKeyword,
    FromKeyword,
    ImportKeyword,
    ConstKeyword,
    ValueKeyword,
    MethodKeyword,
    ClassKeyword,
    StructKeyword,
    InterfaceKeyword,
    NamespaceKeyword,
    FunctionKeyword,
    InitKeyword,
    OperatorKeyword,
    ForKeyword,
    WhileKeyword,
    UntilKeyword,
    DoKeyword,
    ForeachKeyword,
    InKeyword,
    IfKeyword,
    UnlessKeyword,
    ElseKeyword,
    SwitchKeyword,
    ReturnKeyword,
    ThrowKeyword,
    BreakKeyword,
    ContinueKeyword,
    TryKeyword,
    CatchKeyword,
    DeferKeyword,
    WhereKeyword
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
            throw new InvalidOperationException($"Failed to create syntax node: {ShardEngineException.GetLastErrorMessage()}");

        return handle;
    }

    internal static void ThrowIfError(int result)
    {
        if (result != 0)
            throw new InvalidOperationException($"Failed to modify syntax node: {ShardEngineException.GetLastErrorMessage()}");
    }

    internal static IntPtr ToNative(SyntaxNode? node) => node?.Handle ?? IntPtr.Zero;
}

public abstract class SyntaxMemberDeclaration : SyntaxNode
{
    internal SyntaxMemberDeclaration(IntPtr handle) : base(handle) { }

    public virtual SyntaxMemberDeclaration AddModifier(TokenType modifier)
    {
        ThrowIfError(NativeMethods.Shard_AddMemberModifier(Handle, (int)modifier));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddMemberModifier(IntPtr member, int modifierTokenType);
    }
}

public abstract class SyntaxTypeDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxTypeDeclaration(IntPtr handle) : base(handle) { }

    public virtual SyntaxTypeDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(NativeMethods.Shard_AddTypeMember(Handle, member.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddTypeMember(IntPtr type, IntPtr member);
    }
}

public abstract class SyntaxStatement : SyntaxNode
{
    internal SyntaxStatement(IntPtr handle) : base(handle) { }
}

public abstract class SyntaxExpression : SyntaxNode
{
    internal SyntaxExpression(IntPtr handle) : base(handle) { }

    public static SyntaxBinaryExpression operator +(SyntaxExpression left, SyntaxExpression right)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));
        return SyntaxBuilder.Binary(left, right, TokenType.AddOperator);
    }

    public static SyntaxBinaryExpression operator -(SyntaxExpression left, SyntaxExpression right)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));
        return SyntaxBuilder.Binary(left, right, TokenType.SubOperator);
    }

    public static SyntaxBinaryExpression operator *(SyntaxExpression left, SyntaxExpression right)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));
        return SyntaxBuilder.Binary(left, right, TokenType.MultOperator);
    }

    public static SyntaxBinaryExpression operator /(SyntaxExpression left, SyntaxExpression right)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));
        return SyntaxBuilder.Binary(left, right, TokenType.DivOperator);
    }
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

        ThrowIfError(NativeMethods.Shard_SetCompilationUnitNamespace(Handle, ns.Handle));
        return this;
    }

    public SyntaxCompilationUnit SetOrigin(CompilationUnitOrigin origin)
    {
        ThrowIfError(NativeMethods.Shard_SetCompilationUnitOrigin(Handle, (int)origin));
        return this;
    }

    public SyntaxCompilationUnit AddMember(SyntaxMemberDeclaration member)
    {
        if (member == null)
            throw new ArgumentNullException(nameof(member));

        ThrowIfError(NativeMethods.Shard_AddCompilationUnitMember(Handle, member.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetCompilationUnitNamespace(IntPtr unit, IntPtr ns);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetCompilationUnitOrigin(IntPtr unit, int origin);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddCompilationUnitMember(IntPtr unit, IntPtr member);
    }
}

public sealed class SyntaxNamespaceDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxNamespaceDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxNamespaceDeclaration AddIdentifier(string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        ThrowIfError(NativeMethods.Shard_AddNamespaceIdentifier(Handle, name));
        return this;
    }

    public override SyntaxNamespaceDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddNamespaceIdentifier(IntPtr ns, [MarshalAs(UnmanagedType.LPWStr)] string name);
    }
}

public sealed class SyntaxClassDeclaration : SyntaxTypeDeclaration
{
    internal SyntaxClassDeclaration(IntPtr handle) : base(handle) { }

    public override SyntaxClassDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        base.AddMember(member);
        return this;
    }

    public override SyntaxClassDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }
}

public sealed class SyntaxStructDeclaration : SyntaxTypeDeclaration
{
    internal SyntaxStructDeclaration(IntPtr handle) : base(handle) { }

    public override SyntaxStructDeclaration AddMember(SyntaxMemberDeclaration member)
    {
        base.AddMember(member);
        return this;
    }

    public override SyntaxStructDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }
}

public sealed class SyntaxFieldDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxFieldDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxFieldDeclaration SetInitializer(SyntaxExpression? expression)
    {
        ThrowIfError(NativeMethods.Shard_SetFieldInitializer(Handle, ToNative(expression)));
        return this;
    }

    public override SyntaxFieldDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetFieldInitializer(IntPtr field, IntPtr expression);
    }
}

public sealed class SyntaxMethodDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxMethodDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxMethodDeclaration SetReturnType(SyntaxType? returnType)
    {
        ThrowIfError(NativeMethods.Shard_SetMethodReturnType(Handle, ToNative(returnType)));
        return this;
    }

    public SyntaxMethodDeclaration SetParameters(SyntaxParametersList parameters)
    {
        if (parameters == null)
            throw new ArgumentNullException(nameof(parameters));

        ThrowIfError(NativeMethods.Shard_SetMethodParametersList(Handle, parameters.Handle));
        return this;
    }

    public SyntaxMethodDeclaration SetBody(SyntaxStatementsBlock body)
    {
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        ThrowIfError(NativeMethods.Shard_SetMethodBody(Handle, body.Handle));
        return this;
    }

    public override SyntaxMethodDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodReturnType(IntPtr method, IntPtr returnType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodParametersList(IntPtr method, IntPtr parameters);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodBody(IntPtr method, IntPtr body);
    }
}

public sealed class SyntaxConstructorDeclaration : SyntaxMemberDeclaration
{
    internal SyntaxConstructorDeclaration(IntPtr handle) : base(handle) { }

    public SyntaxConstructorDeclaration SetParameters(SyntaxParametersList parameters)
    {
        if (parameters == null)
            throw new ArgumentNullException(nameof(parameters));

        ThrowIfError(NativeMethods.Shard_SetConstructorParametersList(Handle, parameters.Handle));
        return this;
    }

    public SyntaxConstructorDeclaration SetBody(SyntaxStatementsBlock body)
    {
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        ThrowIfError(NativeMethods.Shard_SetConstructorBody(Handle, body.Handle));
        return this;
    }

    public override SyntaxConstructorDeclaration AddModifier(TokenType modifier)
    {
        base.AddModifier(modifier);
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetConstructorParametersList(IntPtr ctor, IntPtr parameters);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetConstructorBody(IntPtr ctor, IntPtr body);
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

        ThrowIfError(NativeMethods.Shard_AddParameter(Handle, name, type.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddParameter(IntPtr list, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);
    }
}

public sealed class SyntaxStatementsBlock : SyntaxNode
{
    internal SyntaxStatementsBlock(IntPtr handle) : base(handle) { }

    public SyntaxStatementsBlock AddStatement(SyntaxStatement statement)
    {
        if (statement == null)
            throw new ArgumentNullException(nameof(statement));

        ThrowIfError(NativeMethods.Shard_AddStatement(Handle, statement.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddStatement(IntPtr block, IntPtr statement);
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

        ThrowIfError(NativeMethods.Shard_SetGenericTypeArguments(Handle, arguments.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetGenericTypeArguments(IntPtr generic, IntPtr arguments);
    }
}

public sealed class SyntaxTypeArgumentsList : SyntaxNode
{
    internal SyntaxTypeArgumentsList(IntPtr handle) : base(handle) { }

    public SyntaxTypeArgumentsList AddArgument(SyntaxType type)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        ThrowIfError(NativeMethods.Shard_AddTypeArgument(Handle, type.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddTypeArgument(IntPtr list, IntPtr type);
    }
}

public sealed class SyntaxInvocationExpression : SyntaxExpression
{
    internal SyntaxInvocationExpression(IntPtr handle) : base(handle) { }

    public SyntaxInvocationExpression SetArguments(SyntaxArgumentsList arguments)
    {
        if (arguments == null)
            throw new ArgumentNullException(nameof(arguments));

        ThrowIfError(NativeMethods.Shard_SetInvocationArgumentsList(Handle, arguments.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInvocationArgumentsList(IntPtr invocation, IntPtr arguments);
    }
}

public sealed class SyntaxObjectExpression : SyntaxExpression
{
    internal SyntaxObjectExpression(IntPtr handle) : base(handle) { }

    public SyntaxObjectExpression SetArguments(SyntaxArgumentsList arguments)
    {
        if (arguments == null)
            throw new ArgumentNullException(nameof(arguments));

        ThrowIfError(NativeMethods.Shard_SetObjectArgumentsList(Handle, arguments.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetObjectArgumentsList(IntPtr objectExpr, IntPtr arguments);
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

        ThrowIfError(NativeMethods.Shard_AddCollectionElement(Handle, element.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddCollectionElement(IntPtr collection, IntPtr element);
    }
}

public sealed class SyntaxArgumentsList : SyntaxNode
{
    internal SyntaxArgumentsList(IntPtr handle) : base(handle) { }

    public SyntaxArgumentsList AddArgument(SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        ThrowIfError(NativeMethods.Shard_AddArgument(Handle, expression.Handle));
        return this;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddArgument(IntPtr list, IntPtr expression);
    }
}

/// <summary>
/// Fluent factory for building ShardScript syntax trees programmatically.
/// </summary>
public static class SyntaxBuilder
{
    public static SyntaxCompilationUnit Unit(CompilationContext context)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        return new SyntaxCompilationUnit(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateCompilationUnit(context.Handle)));
    }

    public static SyntaxNamespaceDeclaration Namespace(SyntaxNode? parent = null)
        => new SyntaxNamespaceDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateNamespaceDeclaration(SyntaxNode.ToNative(parent))));

    public static SyntaxClassDeclaration Class(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxClassDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateClassDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxStructDeclaration Struct(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxStructDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateStructDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxFieldDeclaration Field(SyntaxNode? parent, string name, SyntaxType type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SyntaxFieldDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateFieldDeclaration(SyntaxNode.ToNative(parent), name, type.Handle)));
    }

    public static SyntaxMethodDeclaration Method(SyntaxNode? parent, string name, SyntaxType? returnType = null)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxMethodDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateMethodDeclaration(SyntaxNode.ToNative(parent), name, SyntaxNode.ToNative(returnType))));
    }

    public static SyntaxConstructorDeclaration Constructor(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxConstructorDeclaration(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateConstructorDeclaration(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxParametersList Parameters(SyntaxNode? parent = null)
        => new SyntaxParametersList(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateParametersList(SyntaxNode.ToNative(parent))));

    public static SyntaxStatementsBlock Block(SyntaxNode? parent = null)
        => new SyntaxStatementsBlock(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateStatementsBlock(SyntaxNode.ToNative(parent))));

    public static SyntaxArgumentsList Arguments(SyntaxNode? parent = null)
        => new SyntaxArgumentsList(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateArgumentsList(SyntaxNode.ToNative(parent))));

    public static SyntaxTypeArgumentsList TypeArguments(SyntaxNode? parent = null)
        => new SyntaxTypeArgumentsList(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateTypeArgumentsList(SyntaxNode.ToNative(parent))));

    public static SyntaxPredefinedType PredefinedType(SyntaxNode? parent, TokenType tokenType)
        => new SyntaxPredefinedType(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreatePredefinedType(SyntaxNode.ToNative(parent), (int)tokenType)));

    public static SyntaxIdentifierNameType IdentifierType(SyntaxNode? parent, string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SyntaxIdentifierNameType(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateIdentifierNameType(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxArrayType ArrayType(SyntaxNode? parent, SyntaxType elementType, int rank = 1)
    {
        if (elementType == null)
            throw new ArgumentNullException(nameof(elementType));

        return new SyntaxArrayType(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateArrayType(SyntaxNode.ToNative(parent), elementType.Handle, rank)));
    }

    public static SyntaxNullableType NullableType(SyntaxNode? parent, SyntaxType underlyingType)
    {
        if (underlyingType == null)
            throw new ArgumentNullException(nameof(underlyingType));

        return new SyntaxNullableType(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateNullableType(SyntaxNode.ToNative(parent), underlyingType.Handle)));
    }

    public static SyntaxGenericType GenericType(SyntaxNode? parent, SyntaxType underlyingType)
    {
        if (underlyingType == null)
            throw new ArgumentNullException(nameof(underlyingType));

        return new SyntaxGenericType(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateGenericType(SyntaxNode.ToNative(parent), underlyingType.Handle)));
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

        return new SyntaxVariableStatement(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateVariableStatement(SyntaxNode.ToNative(parent), name, SyntaxNode.ToNative(type), SyntaxNode.ToNative(initializer))));
    }

    public static SyntaxExpressionStatement ExpressionStatement(SyntaxNode? parent, SyntaxExpression expression)
    {
        if (expression == null)
            throw new ArgumentNullException(nameof(expression));

        return new SyntaxExpressionStatement(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateExpressionStatement(SyntaxNode.ToNative(parent), expression.Handle)));
    }

    public static SyntaxReturnStatement Return(SyntaxNode? parent, SyntaxExpression? expression = null)
        => new SyntaxReturnStatement(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateReturnStatement(SyntaxNode.ToNative(parent), SyntaxNode.ToNative(expression))));

    public static SyntaxForEachStatement ForEach(SyntaxNode? parent, string variableName, SyntaxExpression range, SyntaxStatementsBlock body)
    {
        if (variableName == null)
            throw new ArgumentNullException(nameof(variableName));
        if (range == null)
            throw new ArgumentNullException(nameof(range));
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        return new SyntaxForEachStatement(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateForEachStatement(SyntaxNode.ToNative(parent), variableName, range.Handle, body.Handle)));
    }

    public static SyntaxWhileStatement While(SyntaxNode? parent, SyntaxExpression condition, SyntaxStatementsBlock body)
    {
        if (condition == null)
            throw new ArgumentNullException(nameof(condition));
        if (body == null)
            throw new ArgumentNullException(nameof(body));

        return new SyntaxWhileStatement(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateWhileStatement(SyntaxNode.ToNative(parent), condition.Handle, body.Handle)));
    }

    public static SyntaxLiteralExpression Literal(TokenType tokenType, string value, SyntaxNode? parent = null)
    {
        if (value == null)
            throw new ArgumentNullException(nameof(value));

        return new SyntaxLiteralExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateLiteralExpression(SyntaxNode.ToNative(parent), (int)tokenType, value)));
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

        return new SyntaxIdentifierExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateIdentifierExpression(SyntaxNode.ToNative(parent), name)));
    }

    public static SyntaxMemberAccessExpression MemberAccess(SyntaxExpression? previous, string memberName, SyntaxNode? parent = null)
    {
        if (memberName == null)
            throw new ArgumentNullException(nameof(memberName));

        return new SyntaxMemberAccessExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateMemberAccessExpression(SyntaxNode.ToNative(parent), SyntaxNode.ToNative(previous), memberName)));
    }

    public static SyntaxBinaryExpression Binary(SyntaxExpression left, SyntaxExpression right, TokenType operatorTokenType, SyntaxNode? parent = null)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));

        return new SyntaxBinaryExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateBinaryExpression(SyntaxNode.ToNative(parent), left.Handle, right.Handle, (int)operatorTokenType)));
    }

    public static SyntaxUnaryExpression Unary(SyntaxExpression operand, TokenType operatorTokenType, bool postfix = false, SyntaxNode? parent = null)
    {
        if (operand == null)
            throw new ArgumentNullException(nameof(operand));

        return new SyntaxUnaryExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateUnaryExpression(SyntaxNode.ToNative(parent), operand.Handle, (int)operatorTokenType, postfix ? 1 : 0)));
    }

    public static SyntaxInvocationExpression Invocation(SyntaxExpression target, string? methodName = null, SyntaxNode? parent = null)
    {
        if (target == null)
            throw new ArgumentNullException(nameof(target));

        return new SyntaxInvocationExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateInvocationExpression(SyntaxNode.ToNative(parent), target.Handle, methodName)));
    }

    public static SyntaxObjectExpression Object(SyntaxType type, SyntaxNode? parent = null)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SyntaxObjectExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateObjectExpression(SyntaxNode.ToNative(parent), type.Handle)));
    }

    public static SyntaxRangeExpression Range(SyntaxExpression left, SyntaxExpression right, bool inclusive = false, SyntaxNode? parent = null)
    {
        if (left == null)
            throw new ArgumentNullException(nameof(left));
        if (right == null)
            throw new ArgumentNullException(nameof(right));

        return new SyntaxRangeExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateRangeExpression(SyntaxNode.ToNative(parent), left.Handle, right.Handle, inclusive ? 1 : 0)));
    }

    public static SyntaxCollectionExpression Collection(SyntaxNode? parent = null)
        => new SyntaxCollectionExpression(SyntaxNode.CreateHandle(() => NativeMethods.Shard_CreateCollectionExpression(SyntaxNode.ToNative(parent))));

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateCompilationUnit(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateNamespaceDeclaration(IntPtr parent);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateClassDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateStructDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateFieldDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateMethodDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateConstructorDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateParametersList(IntPtr parent);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateStatementsBlock(IntPtr parent);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateArgumentsList(IntPtr parent);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateTypeArgumentsList(IntPtr parent);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreatePredefinedType(IntPtr parent, int tokenType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateIdentifierNameType(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateArrayType(IntPtr parent, IntPtr elementType, int rank);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateNullableType(IntPtr parent, IntPtr underlyingType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateGenericType(IntPtr parent, IntPtr underlyingType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateVariableStatement(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, IntPtr initializer);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateExpressionStatement(IntPtr parent, IntPtr expression);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateReturnStatement(IntPtr parent, IntPtr expression);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateForEachStatement(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string variableName, IntPtr range, IntPtr body);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateWhileStatement(IntPtr parent, IntPtr condition, IntPtr body);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateLiteralExpression(IntPtr parent, int tokenType, [MarshalAs(UnmanagedType.LPWStr)] string value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateIdentifierExpression(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateMemberAccessExpression(IntPtr parent, IntPtr previous, [MarshalAs(UnmanagedType.LPWStr)] string memberName);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateBinaryExpression(IntPtr parent, IntPtr left, IntPtr right, int operatorTokenType);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateUnaryExpression(IntPtr parent, IntPtr operand, int operatorTokenType, int isPostfix);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateInvocationExpression(IntPtr parent, IntPtr target, [MarshalAs(UnmanagedType.LPWStr)] string? methodName);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateObjectExpression(IntPtr parent, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateRangeExpression(IntPtr parent, IntPtr left, IntPtr right, int isInclusive);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateCollectionExpression(IntPtr parent);
    }
}
