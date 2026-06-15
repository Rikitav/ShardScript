using System.Runtime.InteropServices;
using System.Text;

#pragma warning disable SYSLIB1054
namespace ShardScript.NET;

public enum CompilationUnitOrigin
{
    Unknown = 0,
    SourceFile = 1,
    DynamicLib = 2
}

internal static class Native
{
    private const string LibraryName = "libShardScript.dll";

    // Error handling
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetLastError([Out] StringBuilder? buffer, int bufferLen);

    // Compilation Context
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCompilationContext();

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_DestroyCompilationContext(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddLibrary(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string path);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddSource(IntPtr ctx,
        [MarshalAs(UnmanagedType.LPWStr)] string sourceName,
        [MarshalAs(UnmanagedType.LPWStr)] string code,
        int origin);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddSourceFile(IntPtr ctx,
        [MarshalAs(UnmanagedType.LPWStr)] string filePath,
        int origin);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_Analyze(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_Compile(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CompileAndRun(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetEntryPoint(IntPtr ctx, int value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetEntryPoint(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_HasErrors(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetErrorCount(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_ResetDiagnostics(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_GetDiagnostics(IntPtr ctx, [Out] StringBuilder? buffer, int bufferLen);

    // Application Domain
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_RunDomain(IntPtr domain);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_DestroyDomain(IntPtr domain);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetVirtualMachine(IntPtr domain);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetGarbageCollector(IntPtr domain);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetProgram(IntPtr domain);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetEntryPointMethod(IntPtr domain);

    // Virtual Machine
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_VMRun(IntPtr vm);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_VMAbort(IntPtr vm);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_VMTerminateCallStack(IntPtr vm);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_VMInvokeMethod(IntPtr vm, IntPtr method, IntPtr[] args, int argCount);

    // Garbage Collector / Values
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GCFromInteger(IntPtr gc, long value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GCFromDouble(IntPtr gc, double value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GCFromBool(IntPtr gc, int value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GCFromString(IntPtr gc, [MarshalAs(UnmanagedType.LPWStr)] string value);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern long Shard_ReadInteger(IntPtr instance);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern double Shard_ReadDouble(IntPtr instance);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_ReadBool(IntPtr instance);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_ReadString(IntPtr instance);

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
    public static extern int Shard_GetNamespaceIdentifierCount(IntPtr ns);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetNamespaceIdentifier(IntPtr ns, int index);

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
    public static extern IntPtr Shard_FindMethodInType(IntPtr type, [MarshalAs(UnmanagedType.LPWStr)] string name, int parameterCount);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetSymbolName(IntPtr symbol);

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

    // Utility
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_GetVersion();

    // Syntax Builder
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern IntPtr Shard_CreateCompilationUnit(IntPtr ctx);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_AddCompilationUnit(IntPtr ctx, IntPtr unit);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_SetCompilationUnitOrigin(IntPtr unit, int origin);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern int Shard_MarkForReAnalyze(IntPtr ctx);

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
    public static extern IntPtr Shard_CreateClassSymbol(IntPtr ctx, IntPtr parent, [MarshalAs(UnmanagedType.LPWStr)] string name);

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

public static class ShardException
{
    public static string GetLastError()
    {
        int length = Native.Shard_GetLastError(null, 0);
        if (length <= 0)
            return string.Empty;

        StringBuilder sb = new StringBuilder(length + 1);
        Native.Shard_GetLastError(sb, sb.Capacity);
        return sb.ToString();
    }
}

public sealed class CompilationContext : IDisposable
{
    private IntPtr _handle;

    public CompilationContext()
    {
        _handle = Native.Shard_CreateCompilationContext();
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create compilation context: {ShardException.GetLastError()}");
    }

    public IntPtr Handle => _handle;

    private static void ThrowIfError(int result)
    {
        if (result != 0)
            throw new InvalidOperationException($"ShardScript API error: {ShardException.GetLastError()}");
    }

    public void AddLibrary(string path)
    {
        ThrowIfError(Native.Shard_AddLibrary(_handle, path));
    }

    public void AddSource(string name, string code, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        ThrowIfError(Native.Shard_AddSource(_handle, name, code, (int)origin));
    }

    public void AddSourceFile(string filePath, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        ThrowIfError(Native.Shard_AddSourceFile(_handle, filePath, (int)origin));
    }

    public void Analyze()
    {
        ThrowIfError(Native.Shard_Analyze(_handle));
    }

    public ApplicationDomain Compile()
    {
        var domain = Native.Shard_Compile(_handle);
        if (domain == IntPtr.Zero)
            throw new InvalidOperationException($"Compilation failed: {ShardException.GetLastError()}");

        return new ApplicationDomain(domain);
    }

    public ApplicationDomain CompileAndRun()
    {
        var domain = Native.Shard_CompileAndRun(_handle);
        if (domain == IntPtr.Zero)
            throw new InvalidOperationException($"Compilation/run failed: {ShardException.GetLastError()}");

        return new ApplicationDomain(domain);
    }

    public bool EntryPointEnabled
    {
        set => ThrowIfError(Native.Shard_SetEntryPoint(_handle, value ? 1 : 0));
        get => Native.Shard_GetEntryPoint(_handle) != 0;
    }

    public bool HasErrors => Native.Shard_HasErrors(_handle) != 0;

    public int ErrorCount => Native.Shard_GetErrorCount(_handle);

    public void ResetDiagnostics()
    {
        ThrowIfError(Native.Shard_ResetDiagnostics(_handle));
    }

    public string GetDiagnostics()
    {
        int length = Native.Shard_GetDiagnostics(_handle, null, 0);
        if (length <= 0)
            return string.Empty;

        StringBuilder sb = new StringBuilder(length + 1);
        Native.Shard_GetDiagnostics(_handle, sb, sb.Capacity);
        return sb.ToString();
    }

    public IReadOnlyList<CompilationUnit> GetCompilationUnits()
    {
        int count = Native.Shard_GetCompilationUnitCount(_handle);
        CompilationUnit[] units = new CompilationUnit[count];
        for (int i = 0; i < count; i++)
            units[i] = new CompilationUnit(Native.Shard_GetCompilationUnit(_handle, i));

        return units;
    }

    public TypeSymbol? FindType(string name)
    {
        var type = Native.Shard_FindType(_handle, name);
        return type == IntPtr.Zero ? null : new TypeSymbol(type);
    }

    public void AddCompilationUnit(CompilationUnit unit)
    {
        if (unit == null)
            throw new ArgumentNullException(nameof(unit));

        ThrowIfError(Native.Shard_AddCompilationUnit(_handle, unit.Handle));
    }

    public void AddCompilationUnit(SyntaxCompilationUnit unit)
    {
        if (unit == null)
            throw new ArgumentNullException(nameof(unit));

        ThrowIfError(Native.Shard_AddCompilationUnit(_handle, unit.Handle));
    }

    public void MarkForReAnalyze()
    {
        ThrowIfError(Native.Shard_MarkForReAnalyze(_handle));
    }

    public void Dispose()
    {
        if (_handle != IntPtr.Zero)
        {
            Native.Shard_DestroyCompilationContext(_handle);
            _handle = IntPtr.Zero;
        }
    }
}

public sealed class CompilationUnit
{
    public IntPtr Handle { get; }

    internal CompilationUnit(IntPtr handle)
    {
        Handle = handle;
    }

    public CompilationUnitOrigin Origin => (CompilationUnitOrigin)Native.Shard_GetCompilationUnitOrigin(Handle);

    public NamespaceDeclaration? Namespace
    {
        get
        {
            var ns = Native.Shard_GetUnitNamespace(Handle);
            return ns == IntPtr.Zero ? null : new NamespaceDeclaration(ns);
        }
    }

    public IReadOnlyList<ClassDeclaration> GetClasses()
    {
        int count = Native.Shard_GetUnitClassCount(Handle);
        ClassDeclaration[] classes = new ClassDeclaration[count];
        for (int i = 0; i < count; i++)
            classes[i] = new ClassDeclaration(Native.Shard_GetUnitClass(Handle, i));

        return classes;
    }
}

public sealed class NamespaceDeclaration
{
    public IntPtr Handle { get; }

    internal NamespaceDeclaration(IntPtr handle)
    {
        Handle = handle;
    }

    public string Name
    {
        get
        {
            int count = Native.Shard_GetNamespaceIdentifierCount(Handle);
            if (count == 0)
                return string.Empty;

            List<string> parts = new List<string>(count);
            for (int i = 0; i < count; i++)
            {
                var ptr = Native.Shard_GetNamespaceIdentifier(Handle, i);
                parts.Add(ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!);
            }

            return string.Join(".", parts);
        }
    }
}

public sealed class ClassDeclaration
{
    public IntPtr Handle { get; }

    internal ClassDeclaration(IntPtr handle)
    {
        Handle = handle;
    }

    public string Name
    {
        get
        {
            var ptr = Native.Shard_GetTypeName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public IReadOnlyList<MethodSymbol> GetMethods(CompilationContext context)
    {
        int count = Native.Shard_GetTypeMethodCount(context.Handle, Handle);
        MethodSymbol[] methods = new MethodSymbol[count];
        for (int i = 0; i < count; i++)
            methods[i] = new MethodSymbol(Native.Shard_GetTypeMethod(context.Handle, Handle, i));

        return methods;
    }
}

public sealed class TypeSymbol
{
    public IntPtr Handle { get; }

    internal TypeSymbol(IntPtr handle)
    {
        Handle = handle;
    }

    public string Name
    {
        get
        {
            var ptr = Native.Shard_GetSymbolName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public MethodSymbol? FindMethod(string name, int parameterCount)
    {
        var method = Native.Shard_FindMethodInType(Handle, name, parameterCount);
        return method == IntPtr.Zero ? null : new MethodSymbol(method);
    }
}

public sealed class MethodSymbol
{
    public IntPtr Handle { get; }

    internal MethodSymbol(IntPtr handle)
    {
        Handle = handle;
    }

    public string Name
    {
        get
        {
            var ptr = Native.Shard_GetMethodName(Handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    public int ParameterCount => Native.Shard_GetMethodParameterCount(Handle);

    public string GetParameterName(int index)
    {
        var ptr = Native.Shard_GetMethodParameterName(Handle, index);
        return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
    }

    public TypeSymbol? GetParameterType(int index)
    {
        var type = Native.Shard_GetMethodParameterType(Handle, index);
        return type == IntPtr.Zero ? null : new TypeSymbol(type);
    }

    public TypeSymbol? ReturnType
    {
        get
        {
            var type = Native.Shard_GetMethodReturnType(Handle);
            return type == IntPtr.Zero ? null : new TypeSymbol(type);
        }
    }

    public bool IsStatic => Native.Shard_IsMethodStatic(Handle) != 0;
}

public sealed class ApplicationDomain : IDisposable
{
    private IntPtr _handle;

    internal ApplicationDomain(IntPtr handle)
    {
        _handle = handle;
    }

    public IntPtr Handle => _handle;

    public void Run()
    {
        int result = Native.Shard_RunDomain(_handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to run domain: {ShardException.GetLastError()}");
    }

    public VirtualMachine GetVirtualMachine()
    {
        var vm = Native.Shard_GetVirtualMachine(_handle);
        if (vm == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to get virtual machine: {ShardException.GetLastError()}");

        return new VirtualMachine(vm);
    }

    public GarbageCollector GetGarbageCollector()
    {
        var gc = Native.Shard_GetGarbageCollector(_handle);
        if (gc == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to get garbage collector: {ShardException.GetLastError()}");

        return new GarbageCollector(gc);
    }

    public MethodSymbol? EntryPointMethod
    {
        get
        {
            var method = Native.Shard_GetEntryPointMethod(_handle);
            return method == IntPtr.Zero ? null : new MethodSymbol(method);
        }
    }

    public void Dispose()
    {
        if (_handle != IntPtr.Zero)
        {
            Native.Shard_DestroyDomain(_handle);
            _handle = IntPtr.Zero;
        }
    }
}

public sealed class VirtualMachine
{
    private readonly IntPtr _handle;

    internal VirtualMachine(IntPtr handle)
    {
        _handle = handle;
    }

    public IntPtr Handle => _handle;

    public void Run()
    {
        int result = Native.Shard_VMRun(_handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to run VM: {ShardException.GetLastError()}");
    }

    public void Abort()
    {
        Native.Shard_VMAbort(_handle);
    }

    public void TerminateCallStack()
    {
        Native.Shard_VMTerminateCallStack(_handle);
    }

    public ObjectInstance InvokeMethod(MethodSymbol method, params ObjectInstance[] args)
    {
        if (method == null)
            throw new ArgumentNullException(nameof(method));

        var argHandles = new IntPtr[args.Length];
        for (int i = 0; i < args.Length; i++)
            argHandles[i] = args[i].Handle;

        var result = Native.Shard_VMInvokeMethod(_handle, method.Handle, argHandles, argHandles.Length);
        return new ObjectInstance(result);
    }
}

public sealed class GarbageCollector
{
    private readonly IntPtr _handle;

    internal GarbageCollector(IntPtr handle)
    {
        _handle = handle;
    }

    public IntPtr Handle => _handle;

    public ObjectInstance FromInteger(long value)
    {
        return new ObjectInstance(Native.Shard_GCFromInteger(_handle, value));
    }

    public ObjectInstance FromDouble(double value)
    {
        return new ObjectInstance(Native.Shard_GCFromDouble(_handle, value));
    }

    public ObjectInstance FromBool(bool value)
    {
        return new ObjectInstance(Native.Shard_GCFromBool(_handle, value ? 1 : 0));
    }

    public ObjectInstance FromString(string value)
    {
        return new ObjectInstance(Native.Shard_GCFromString(_handle, value));
    }
}

public readonly struct ObjectInstance
{
    public IntPtr Handle { get; }

    public ObjectInstance(IntPtr handle)
    {
        Handle = handle;
    }

    public bool IsValid => Handle != IntPtr.Zero;

    public long AsInteger()
    {
        return Native.Shard_ReadInteger(Handle);
    }

    public double AsDouble()
    {
        return Native.Shard_ReadDouble(Handle);
    }

    public bool AsBool()
    {
        return Native.Shard_ReadBool(Handle) != 0;
    }

    public string AsString()
    {
        var ptr = Native.Shard_ReadString(Handle);
        return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
    }
}

public static class ShardInfo
{
    public static string Version
    {
        get
        {
            var ptr = Native.Shard_GetVersion();
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }
}

#pragma warning restore SYSLIB1054
