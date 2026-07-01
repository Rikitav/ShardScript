#pragma once

// ============================================================================
// ShardScript Public C/C++ API Header
// ============================================================================
// This header declares every `Shard_*` function exported by the ShardScript
// engine.  The functions have C linkage (`extern "C"`) and use raw pointers so
// they can be consumed from C, C++ and via P/Invoke from other languages.
//
// Include `<ShardScript.hpp>` for the complete public API, or include this
// header directly if you only need the flat C-style entry points.
// ============================================================================

#include <cstddef>
#include <cstdint>

#include <shard/ShardScriptAPI.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>          // ShardManagedMethodCallback
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>   // CompilationUnitOrigin

namespace shard
{
    // Core
    class CompilationContext;
    class ApplicationDomain;
    class VirtualMachine;
    class GarbageCollector;
    class ProgramVirtualImage;
    class ObjectInstance;

    // Semantic / symbols
    class SymbolTable;
    class SyntaxSymbol;
    class TypeSymbol;
    class TypeDeclarationSyntax;
    class InterfaceSymbol;
    class MethodSymbol;
    class ParameterSymbol;
    class FieldSymbol;
    class NamespaceSymbol;
    class ClassSymbol;

    // Syntax / nodes
    class SyntaxNode;
    class CompilationUnitSyntax;
    class NamespaceDeclarationSyntax;
    class MemberDeclarationSyntax;
    class ClassDeclarationSyntax;
    class StructDeclarationSyntax;
    class FieldDeclarationSyntax;
    class MethodDeclarationSyntax;
    class ConstructorDeclarationSyntax;
    class ParametersListSyntax;
    class StatementsBlockSyntax;
    class StatementSyntax;
    class ExpressionSyntax;
    class TypeSyntax;
    class PredefinedTypeSyntax;
    class IdentifierNameTypeSyntax;
    class ArrayTypeSyntax;
    class NullableTypeSyntax;
    class GenericTypeSyntax;
    class TypeArgumentsListSyntax;
    class VariableStatementSyntax;
    class ExpressionStatementSyntax;
    class ReturnStatementSyntax;
    class ForEachStatementSyntax;
    class WhileStatementSyntax;
    class LiteralExpressionSyntax;
    class MemberAccessExpressionSyntax;
    class BinaryExpressionSyntax;
    class UnaryExpressionSyntax;
    class InvokationExpressionSyntax;
    class ObjectExpressionSyntax;
    class RangeExpressionSyntax;
    class CollectionExpressionSyntax;
    class ArgumentsListSyntax;
    class DeferStatementSyntax;
    class BreakStatementSyntax;
    class ContinueStatementSyntax;
    class ThrowStatementSyntax;
    class TryStatementSyntax;
    class CatchClauseSyntax;
}

extern "C"
{
    // =========================================================================
    // Error Handling
    // =========================================================================

    SHARD_API int Shard_GetLastError(wchar_t* buffer, int bufferLen);

    // =========================================================================
    // Compilation Context API
    // =========================================================================

    SHARD_API shard::CompilationContext* Shard_CreateCompilationContext();
    SHARD_API int Shard_DestroyCompilationContext(shard::CompilationContext* ctx);
    SHARD_API int Shard_AddLibrary(shard::CompilationContext* ctx, const wchar_t* path);
    SHARD_API int Shard_AddSource(shard::CompilationContext* ctx, const wchar_t* sourceName, const wchar_t* code, shard::CompilationUnitOrigin origin);
    SHARD_API int Shard_AddSourceFile(shard::CompilationContext* ctx, const wchar_t* filePath, shard::CompilationUnitOrigin origin);
    SHARD_API int Shard_Analyze(shard::CompilationContext* ctx);
    SHARD_API shard::ApplicationDomain* Shard_Compile(shard::CompilationContext* ctx);
    SHARD_API shard::ApplicationDomain* Shard_CompileAndRun(shard::CompilationContext* ctx);
    SHARD_API int Shard_SetEntryPoint(shard::CompilationContext* ctx, int value);
    SHARD_API int Shard_GetEntryPoint(shard::CompilationContext* ctx);

    // =========================================================================
    // Diagnostics API
    // =========================================================================

    SHARD_API int Shard_HasErrors(shard::CompilationContext* ctx);
    SHARD_API int Shard_GetErrorCount(shard::CompilationContext* ctx);
    SHARD_API int Shard_GetWarningCount(shard::CompilationContext* ctx);
    SHARD_API int Shard_ResetDiagnostics(shard::CompilationContext* ctx);
    SHARD_API int Shard_GetDiagnostics(shard::CompilationContext* ctx, wchar_t* buffer, int bufferLen);

    SHARD_API int Shard_GetDiagnosticCount(shard::CompilationContext* ctx);
    SHARD_API int Shard_GetDiagnosticSeverity(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticLine(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticColumn(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticLength(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticMessage(shard::CompilationContext* ctx, int index, wchar_t* buffer, int bufferLen);

    // =========================================================================
    // Application Domain API
    // =========================================================================

    SHARD_API int Shard_RunDomain(shard::ApplicationDomain* domain);
    SHARD_API int Shard_DestroyDomain(shard::ApplicationDomain* domain);
    SHARD_API shard::VirtualMachine* Shard_GetVirtualMachine(shard::ApplicationDomain* domain);
    SHARD_API shard::GarbageCollector* Shard_GetGarbageCollector(shard::ApplicationDomain* domain);
    SHARD_API shard::ProgramVirtualImage* Shard_GetProgram(shard::ApplicationDomain* domain);
    SHARD_API shard::MethodSymbol* Shard_GetEntryPointMethod(shard::ApplicationDomain* domain);

    // =========================================================================
    // Virtual Machine API
    // =========================================================================

    SHARD_API int Shard_VMRun(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMAbort(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMTerminateCallStack(shard::VirtualMachine* vm);
    SHARD_API shard::ObjectInstance* Shard_VMInvokeMethod(shard::VirtualMachine* vm, shard::MethodSymbol* method, shard::ObjectInstance** args, int argCount);

    // =========================================================================
    // Garbage Collector / Value API
    // =========================================================================

    SHARD_API shard::ObjectInstance* Shard_GCFromInteger(shard::GarbageCollector* gc, std::int64_t value);
    SHARD_API shard::ObjectInstance* Shard_GCFromDouble(shard::GarbageCollector* gc, double value);
    SHARD_API shard::ObjectInstance* Shard_GCFromBool(shard::GarbageCollector* gc, int value);
    SHARD_API shard::ObjectInstance* Shard_GCFromString(shard::GarbageCollector* gc, const wchar_t* value);

    SHARD_API std::int64_t Shard_ReadInteger(shard::ObjectInstance* instance);
    SHARD_API double Shard_ReadDouble(shard::ObjectInstance* instance);
    SHARD_API int Shard_ReadBool(shard::ObjectInstance* instance);
    SHARD_API const wchar_t* Shard_ReadString(shard::ObjectInstance* instance);

    // =========================================================================
    // Symbol Inspection API
    // =========================================================================

    SHARD_API int Shard_GetCompilationUnitCount(shard::CompilationContext* ctx);
    SHARD_API shard::CompilationUnitSyntax* Shard_GetCompilationUnit(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetCompilationUnitOrigin(shard::CompilationUnitSyntax* unit);
    SHARD_API shard::NamespaceDeclarationSyntax* Shard_GetUnitNamespace(shard::CompilationUnitSyntax* unit);

    SHARD_API int Shard_GetNamespaceIdentifierCount(shard::NamespaceDeclarationSyntax* ns);
    SHARD_API const wchar_t* Shard_GetNamespaceIdentifier(shard::NamespaceDeclarationSyntax* ns, int index);

    SHARD_API int Shard_GetUnitClassCount(shard::CompilationUnitSyntax* unit);
    SHARD_API shard::ClassDeclarationSyntax* Shard_GetUnitClass(shard::CompilationUnitSyntax* unit, int index);

    SHARD_API const wchar_t* Shard_GetTypeName(shard::TypeDeclarationSyntax* type);

    SHARD_API int Shard_GetTypeMethodCount(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type);
    SHARD_API shard::MethodSymbol* Shard_GetTypeMethod(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type, int index);

    SHARD_API int Shard_GetTypeFieldCount(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type);
    SHARD_API shard::FieldSymbol* Shard_GetTypeField(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type, int index);

    SHARD_API int Shard_GetTypeInterfaceCount(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type);
    SHARD_API shard::TypeSymbol* Shard_GetTypeInterface(shard::CompilationContext* ctx, shard::TypeDeclarationSyntax* type, int index);

    SHARD_API int Shard_GetSymbolTableTypeCount(shard::CompilationContext* ctx);
    SHARD_API shard::TypeSymbol* Shard_GetSymbolTableType(shard::CompilationContext* ctx, int index);
    SHARD_API shard::TypeSymbol* Shard_FindType(shard::CompilationContext* ctx, const wchar_t* name);
    SHARD_API shard::MethodSymbol* Shard_FindMethodInType(shard::TypeSymbol* type, const wchar_t* name, int parameterCount);

    SHARD_API shard::InterfaceSymbol* Shard_GetStandardInterface(shard::CompilationContext* ctx, int kind);
    SHARD_API int Shard_IsTypeAssignableFrom(shard::TypeSymbol* target, shard::TypeSymbol* source);

    SHARD_API const wchar_t* Shard_GetSymbolName(shard::SyntaxSymbol* symbol);
    SHARD_API const wchar_t* Shard_GetMethodName(shard::MethodSymbol* method);
    SHARD_API int Shard_GetMethodParameterCount(shard::MethodSymbol* method);
    SHARD_API const wchar_t* Shard_GetMethodParameterName(shard::MethodSymbol* method, int index);
    SHARD_API shard::TypeSymbol* Shard_GetMethodParameterType(shard::MethodSymbol* method, int index);
    SHARD_API shard::TypeSymbol* Shard_GetMethodReturnType(shard::MethodSymbol* method);
    SHARD_API int Shard_IsMethodStatic(shard::MethodSymbol* method);
    SHARD_API int Shard_GetMethodHandleType(shard::MethodSymbol* method);
    SHARD_API int Shard_SetMethodHandleType(shard::MethodSymbol* method, int handleType);

    SHARD_API shard::TypeSymbol* Shard_GetFieldType(shard::FieldSymbol* field);
    SHARD_API int Shard_IsFieldStatic(shard::FieldSymbol* field);
    SHARD_API const wchar_t* Shard_GetFieldName(shard::FieldSymbol* field);
    SHARD_API shard::FieldSymbol* Shard_FindFieldInType(shard::TypeSymbol* type, const wchar_t* name);

    // =========================================================================
    // Runtime Field Access API
    // =========================================================================

    SHARD_API shard::ObjectInstance* Shard_GCGetStaticField(shard::GarbageCollector* gc, shard::FieldSymbol* field);
    SHARD_API int Shard_GCSetStaticField(shard::GarbageCollector* gc, shard::FieldSymbol* field, shard::ObjectInstance* value);

    // =========================================================================
    // Runtime Object Allocation API
    // =========================================================================

    SHARD_API shard::ObjectInstance* Shard_GCAllocateInstance(shard::GarbageCollector* gc, shard::TypeSymbol* type);
    SHARD_API shard::ObjectInstance* Shard_GCAllocateArray(shard::GarbageCollector* gc, shard::TypeSymbol* elementType, std::size_t length);

    // =========================================================================
    // Runtime Instance Field / Element Access API
    // =========================================================================

    SHARD_API shard::ObjectInstance* Shard_GetInstanceField(shard::ObjectInstance* instance, shard::FieldSymbol* field);
    SHARD_API int Shard_SetInstanceField(shard::ObjectInstance* instance, shard::FieldSymbol* field, shard::ObjectInstance* value);

    SHARD_API shard::ObjectInstance* Shard_GetArrayElement(shard::ObjectInstance* array, std::size_t index);
    SHARD_API int Shard_SetArrayElement(shard::ObjectInstance* array, std::size_t index, shard::ObjectInstance* value);

    // GC-less typed primitive setters: box the value into a transient instance
    // typed as field->ReturnType and write it inline. Reference types (e.g.
    // string) must still go through Shard_SetInstanceField with a GC-owned value.
    SHARD_API int Shard_SetInstanceFieldInteger(shard::ObjectInstance* instance, shard::FieldSymbol* field, std::int64_t value);
    SHARD_API int Shard_SetInstanceFieldDouble(shard::ObjectInstance* instance, shard::FieldSymbol* field, double value);
    SHARD_API int Shard_SetInstanceFieldBool(shard::ObjectInstance* instance, shard::FieldSymbol* field, int value);
    SHARD_API int Shard_SetInstanceFieldChar(shard::ObjectInstance* instance, shard::FieldSymbol* field, wchar_t value);


    // =========================================================================
    // Utility API
    // =========================================================================

    SHARD_API const wchar_t* Shard_GetVersion();

    // =========================================================================
    // Syntax Builder API
    // =========================================================================

    SHARD_API shard::CompilationUnitSyntax* Shard_CreateCompilationUnit(shard::CompilationContext* ctx);
    SHARD_API int Shard_AddCompilationUnit(shard::CompilationContext* ctx, shard::CompilationUnitSyntax* unit);
    SHARD_API int Shard_MarkForReAnalyze(shard::CompilationContext* ctx);
    SHARD_API int Shard_SetCompilationUnitOrigin(shard::CompilationUnitSyntax* unit, shard::CompilationUnitOrigin origin);
    SHARD_API int Shard_SetCompilationUnitNamespace(shard::CompilationUnitSyntax* unit, shard::NamespaceDeclarationSyntax* ns);
    SHARD_API int Shard_AddCompilationUnitMember(shard::CompilationUnitSyntax* unit, shard::MemberDeclarationSyntax* member);

    SHARD_API shard::NamespaceDeclarationSyntax* Shard_CreateNamespaceDeclaration(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddNamespaceIdentifier(shard::NamespaceDeclarationSyntax* ns, const wchar_t* name);

    SHARD_API int Shard_AddMemberModifier(shard::MemberDeclarationSyntax* member, int modifierTokenType);

    SHARD_API shard::ClassDeclarationSyntax* Shard_CreateClassDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API shard::StructDeclarationSyntax* Shard_CreateStructDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_AddTypeMember(shard::TypeDeclarationSyntax* type, shard::MemberDeclarationSyntax* member);

    SHARD_API shard::FieldDeclarationSyntax* Shard_CreateFieldDeclaration(shard::SyntaxNode* parent, const wchar_t* name, shard::TypeSyntax* type);
    SHARD_API int Shard_SetFieldInitializer(shard::FieldDeclarationSyntax* field, shard::ExpressionSyntax* expression);

    SHARD_API shard::MethodDeclarationSyntax* Shard_CreateMethodDeclaration(shard::SyntaxNode* parent, const wchar_t* name, shard::TypeSyntax* returnType);
    SHARD_API shard::ConstructorDeclarationSyntax* Shard_CreateConstructorDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_SetMethodReturnType(shard::MethodDeclarationSyntax* method, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetMethodParametersList(shard::MethodDeclarationSyntax* method, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetMethodBody(shard::MethodDeclarationSyntax* method, shard::StatementsBlockSyntax* body);
    SHARD_API int Shard_SetConstructorParametersList(shard::ConstructorDeclarationSyntax* ctor, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetConstructorBody(shard::ConstructorDeclarationSyntax* ctor, shard::StatementsBlockSyntax* body);

    SHARD_API shard::ParametersListSyntax* Shard_CreateParametersList(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddParameter(shard::ParametersListSyntax* list, const wchar_t* name, shard::TypeSyntax* type);

    SHARD_API shard::StatementsBlockSyntax* Shard_CreateStatementsBlock(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddStatement(shard::StatementsBlockSyntax* block, shard::StatementSyntax* statement);

    SHARD_API shard::PredefinedTypeSyntax* Shard_CreatePredefinedType(shard::SyntaxNode* parent, int tokenType);
    SHARD_API shard::IdentifierNameTypeSyntax* Shard_CreateIdentifierNameType(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API shard::ArrayTypeSyntax* Shard_CreateArrayType(shard::SyntaxNode* parent, shard::TypeSyntax* elementType, int rank);
    SHARD_API shard::NullableTypeSyntax* Shard_CreateNullableType(shard::SyntaxNode* parent, shard::TypeSyntax* underlayingType);
    SHARD_API shard::GenericTypeSyntax* Shard_CreateGenericType(shard::SyntaxNode* parent, shard::TypeSyntax* underlayingType);
    SHARD_API shard::TypeArgumentsListSyntax* Shard_CreateTypeArgumentsList(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddTypeArgument(shard::TypeArgumentsListSyntax* list, shard::TypeSyntax* type);
    SHARD_API int Shard_SetGenericTypeArguments(shard::GenericTypeSyntax* generic, shard::TypeArgumentsListSyntax* arguments);

    SHARD_API shard::VariableStatementSyntax* Shard_CreateVariableStatement(shard::SyntaxNode* parent, const wchar_t* name, shard::TypeSyntax* type, shard::ExpressionSyntax* initializer);
    SHARD_API shard::ExpressionStatementSyntax* Shard_CreateExpressionStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API shard::ReturnStatementSyntax* Shard_CreateReturnStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API shard::DeferStatementSyntax* Shard_CreateDeferStatement(shard::SyntaxNode* parent, shard::StatementSyntax* statement);
    SHARD_API shard::BreakStatementSyntax* Shard_CreateBreakStatement(shard::SyntaxNode* parent);
    SHARD_API shard::ContinueStatementSyntax* Shard_CreateContinueStatement(shard::SyntaxNode* parent);
    SHARD_API shard::ThrowStatementSyntax* Shard_CreateThrowStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API shard::TryStatementSyntax* Shard_CreateTryStatement(shard::SyntaxNode* parent, shard::StatementsBlockSyntax* tryBlock);
    SHARD_API int Shard_AddCatchClause(shard::TryStatementSyntax* tryStmt, const wchar_t* variableName, shard::TypeSyntax* exceptionType, shard::StatementsBlockSyntax* body);

    SHARD_API shard::ForEachStatementSyntax* Shard_CreateForEachStatement(shard::SyntaxNode* parent, const wchar_t* variableName, shard::ExpressionSyntax* range, shard::StatementsBlockSyntax* body);
    SHARD_API shard::WhileStatementSyntax* Shard_CreateWhileStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::StatementsBlockSyntax* body);

    SHARD_API shard::LiteralExpressionSyntax* Shard_CreateLiteralExpression(shard::SyntaxNode* parent, int tokenType, const wchar_t* value);
    SHARD_API shard::MemberAccessExpressionSyntax* Shard_CreateIdentifierExpression(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API shard::MemberAccessExpressionSyntax* Shard_CreateMemberAccessExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* previous, const wchar_t* memberName);
    SHARD_API shard::BinaryExpressionSyntax* Shard_CreateBinaryExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* left, shard::ExpressionSyntax* right, int operatorTokenType);
    SHARD_API shard::UnaryExpressionSyntax* Shard_CreateUnaryExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* operand, int operatorTokenType, int isPostfix);
    SHARD_API shard::InvokationExpressionSyntax* Shard_CreateInvocationExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* target, const wchar_t* methodName);
    SHARD_API int Shard_SetInvocationArgumentsList(shard::InvokationExpressionSyntax* invocation, shard::ArgumentsListSyntax* arguments);
    SHARD_API int Shard_SetInvocationAsExtension(shard::InvokationExpressionSyntax* invocation, int value);
    SHARD_API int Shard_IsInvocationExtension(shard::InvokationExpressionSyntax* invocation);
    SHARD_API shard::ObjectExpressionSyntax* Shard_CreateObjectExpression(shard::SyntaxNode* parent, shard::TypeSyntax* type);
    SHARD_API int Shard_SetObjectArgumentsList(shard::ObjectExpressionSyntax* objectExpr, shard::ArgumentsListSyntax* arguments);
    SHARD_API shard::RangeExpressionSyntax* Shard_CreateRangeExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* left, shard::ExpressionSyntax* right, int isInclusive);
    SHARD_API shard::CollectionExpressionSyntax* Shard_CreateCollectionExpression(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddCollectionElement(shard::CollectionExpressionSyntax* collection, shard::ExpressionSyntax* element);

    SHARD_API shard::ArgumentsListSyntax* Shard_CreateArgumentsList(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddArgument(shard::ArgumentsListSyntax* list, shard::ExpressionSyntax* expression);

    // =========================================================================
    // Symbol Builder API
    // =========================================================================

    SHARD_API shard::SymbolTable* Shard_GetSymbolTable(shard::CompilationContext* ctx);
    SHARD_API shard::TypeSymbol* Shard_GetPrimitiveType(shard::CompilationContext* ctx, int primitiveKind);
    SHARD_API shard::NamespaceSymbol* Shard_CreateNamespaceSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name);
    SHARD_API shard::ClassSymbol* Shard_CreateClassSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name);
    SHARD_API shard::MethodSymbol* Shard_CreateMethodSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* returnType, int isStatic, int accessibility);
    SHARD_API shard::ParameterSymbol* Shard_CreateParameterSymbol(shard::CompilationContext* ctx, const wchar_t* name, shard::TypeSymbol* type);
    SHARD_API int Shard_AddMethodParameter(shard::MethodSymbol* method, shard::ParameterSymbol* parameter);
    SHARD_API shard::FieldSymbol* Shard_CreateFieldSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* type, int isStatic, int accessibility);
    SHARD_API int Shard_SetSymbolAccesibility(shard::SyntaxSymbol* symbol, int accessibility);
}
