using System.Runtime.InteropServices;

namespace ShardScript.NET;

internal static class ShardScriptAPI
{
    public const string LibraryName = "ShardScript.dll";

    public static string Version
    {
        get
        {
            IntPtr ptr = NativeMethods.Shard_GetVersion();
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    private static class NativeMethods
    {
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetVersion();
    }

    // Symbol Inspection
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetCompilationUnitCount(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetCompilationUnit(IntPtr ctx, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetCompilationUnitOrigin(IntPtr unit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetUnitNamespace(IntPtr unit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetUnitClassCount(IntPtr unit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetUnitClass(IntPtr unit, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetTypeName(IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetTypeMethodCount(IntPtr ctx, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetTypeMethod(IntPtr ctx, IntPtr type, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetSymbolTableTypeCount(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetSymbolTableType(IntPtr ctx, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_FindType(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodName(IntPtr method);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetMethodParameterCount(IntPtr method);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodParameterName(IntPtr method, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodParameterType(IntPtr method, int index);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodReturnType(IntPtr method);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_IsMethodStatic(IntPtr method);

    // Syntax Builder
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCompilationUnit(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCompilationUnit(IntPtr ctx, IntPtr unit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetCompilationUnitOrigin(IntPtr unit, int origin);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetCompilationUnitNamespace(IntPtr unit, IntPtr ns);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCompilationUnitMember(IntPtr unit, IntPtr member);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateNamespaceDeclaration(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddNamespaceIdentifier(IntPtr ns, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddMemberModifier(IntPtr member, int modifierTokenType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateClassDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateStructDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddTypeMember(IntPtr type, IntPtr member);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateFieldDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetFieldInitializer(IntPtr field, IntPtr expression);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateMethodDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateConstructorDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodReturnType(IntPtr method, IntPtr returnType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodParametersList(IntPtr method, IntPtr parameters);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodBody(IntPtr method, IntPtr body);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetConstructorParametersList(IntPtr ctor, IntPtr parameters);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetConstructorBody(IntPtr ctor, IntPtr body);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateParametersList(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddParameter(IntPtr list, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateStatementsBlock(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddStatement(IntPtr block, IntPtr statement);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreatePredefinedType(IntPtr parent, int tokenType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateIdentifierNameType(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateArrayType(IntPtr parent, IntPtr elementType, int rank);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateNullableType(IntPtr parent, IntPtr underlyingType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateGenericType(IntPtr parent, IntPtr underlyingType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateTypeArgumentsList(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddTypeArgument(IntPtr list, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetGenericTypeArguments(IntPtr generic, IntPtr arguments);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateVariableStatement(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, IntPtr initializer);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateExpressionStatement(IntPtr parent, IntPtr expression);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateReturnStatement(IntPtr parent, IntPtr expression);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateForEachStatement(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string variableName, IntPtr range, IntPtr body);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateWhileStatement(IntPtr parent, IntPtr condition, IntPtr body);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateLiteralExpression(IntPtr parent, int tokenType, [MarshalAs(UnmanagedType.LPWStr)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateIdentifierExpression(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateMemberAccessExpression(IntPtr parent, IntPtr previous, [MarshalAs(UnmanagedType.LPWStr)] string memberName);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateBinaryExpression(IntPtr parent, IntPtr left, IntPtr right, int operatorTokenType);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateUnaryExpression(IntPtr parent, IntPtr operand, int operatorTokenType, int isPostfix);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateInvocationExpression(IntPtr parent, IntPtr target, [MarshalAs(UnmanagedType.LPWStr)] string? methodName);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetInvocationArgumentsList(IntPtr invocation, IntPtr arguments);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateObjectExpression(IntPtr parent, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetObjectArgumentsList(IntPtr objectExpr, IntPtr arguments);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateRangeExpression(IntPtr parent, IntPtr left, IntPtr right, int isInclusive);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCollectionExpression(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCollectionElement(IntPtr collection, IntPtr element);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateArgumentsList(IntPtr parent);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddArgument(IntPtr list, IntPtr expression);

    // Symbol Builder
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetSymbolTable(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetPrimitiveType(IntPtr ctx, int primitiveKind);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateNamespaceSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateMethodSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType, int isStatic, int accessibility);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateParameterSymbol(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddMethodParameter(IntPtr method, IntPtr parameter);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateFieldSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, int isStatic);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodManagedCallback(IntPtr method, ShardManagedMethodCallbackNative callback, IntPtr userData);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetSymbolAccesibility(IntPtr symbol, int accessibility);
}
