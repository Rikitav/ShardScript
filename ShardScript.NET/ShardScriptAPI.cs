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
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetVersion();
    }

    // Symbol Inspection
    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetCompilationUnitCount(IntPtr ctx);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetCompilationUnit(IntPtr ctx, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetCompilationUnitOrigin(IntPtr unit);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetUnitNamespace(IntPtr unit);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetUnitClassCount(IntPtr unit);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetUnitClass(IntPtr unit, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetTypeName(IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetTypeMethodCount(IntPtr ctx, IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetTypeMethod(IntPtr ctx, IntPtr type, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetSymbolTableTypeCount(IntPtr ctx);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetSymbolTableType(IntPtr ctx, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_FindType(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodName(IntPtr method);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetMethodParameterCount(IntPtr method);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodParameterName(IntPtr method, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodParameterType(IntPtr method, int index);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetMethodReturnType(IntPtr method);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_IsMethodStatic(IntPtr method);

    // Syntax Builder
    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCompilationUnit(IntPtr ctx);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCompilationUnit(IntPtr ctx, IntPtr unit);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetCompilationUnitOrigin(IntPtr unit, int origin);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetCompilationUnitNamespace(IntPtr unit, IntPtr ns);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCompilationUnitMember(IntPtr unit, IntPtr member);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateNamespaceDeclaration(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddNamespaceIdentifier(IntPtr ns, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddMemberModifier(IntPtr member, int modifierTokenType);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateClassDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateStructDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddTypeMember(IntPtr type, IntPtr member);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateFieldDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetFieldInitializer(IntPtr field, IntPtr expression);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateMethodDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateConstructorDeclaration(IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodReturnType(IntPtr method, IntPtr returnType);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodParametersList(IntPtr method, IntPtr parameters);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetMethodBody(IntPtr method, IntPtr body);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetConstructorParametersList(IntPtr ctor, IntPtr parameters);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetConstructorBody(IntPtr ctor, IntPtr body);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateParametersList(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddParameter(IntPtr list, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateStatementsBlock(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddStatement(IntPtr block, IntPtr statement);

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
    public static extern IntPtr Shard_CreateTypeArgumentsList(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddTypeArgument(IntPtr list, IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetGenericTypeArguments(IntPtr generic, IntPtr arguments);

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
    public static extern int Shard_SetInvocationArgumentsList(IntPtr invocation, IntPtr arguments);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateObjectExpression(IntPtr parent, IntPtr type);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetObjectArgumentsList(IntPtr objectExpr, IntPtr arguments);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateRangeExpression(IntPtr parent, IntPtr left, IntPtr right, int isInclusive);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCollectionExpression(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCollectionElement(IntPtr collection, IntPtr element);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateArgumentsList(IntPtr parent);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddArgument(IntPtr list, IntPtr expression);

    // Symbol Builder
    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetSymbolTable(IntPtr ctx);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetPrimitiveType(IntPtr ctx, int primitiveKind);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateNamespaceSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateMethodSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType, int isStatic, int accessibility);

    [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateFieldSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, int isStatic);
}
