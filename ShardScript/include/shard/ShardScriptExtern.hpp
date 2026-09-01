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

#include <ShardScript.hpp>
#include <shard/ShardScriptLIB.hpp>

#include <cstddef>
#include <cstdint>

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
    SHARD_API int Shard_AddLibraries(shard::CompilationContext* ctx, const wchar_t* const* paths, std::size_t count);
    SHARD_API int Shard_AddSource(shard::CompilationContext* ctx, const wchar_t* sourceName, const wchar_t* code, shard::CompilationUnitOrigin origin);
    SHARD_API int Shard_AddSourceFile(shard::CompilationContext* ctx, const wchar_t* filePath, shard::CompilationUnitOrigin origin);
    SHARD_API int Shard_Analyze(shard::CompilationContext* ctx);
    SHARD_API shard::ApplicationDomain* Shard_Compile(shard::CompilationContext* ctx);
    SHARD_API shard::ApplicationDomain* Shard_CompileAndRun(shard::CompilationContext* ctx);
    SHARD_API int Shard_SetEntryPoint(shard::CompilationContext* ctx, int value);
    SHARD_API int Shard_GetEntryPoint(shard::CompilationContext* ctx);
    SHARD_API int Shard_SetPopExpressionStatement(shard::CompilationContext* ctx, int value);

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
    // NOTE: Returns the byte offset of the diagnostic token; ShardScript's TextLocation currently has no true column field.
    SHARD_API int Shard_GetDiagnosticColumn(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticLength(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetDiagnosticMessage(shard::CompilationContext* ctx, int index, wchar_t* buffer, int bufferLen);

    SHARD_API int Shard_ReportError(shard::CompilationContext* ctx, const wchar_t* message, int line, int offset, int length);
    SHARD_API int Shard_ReportWarning(shard::CompilationContext* ctx, const wchar_t* message, int line, int offset, int length);
    SHARD_API int Shard_ReportInfo(shard::CompilationContext* ctx, const wchar_t* message, int line, int offset, int length);

    // =========================================================================
    // Application Domain API
    // =========================================================================

    SHARD_API int Shard_RunDomain(shard::ApplicationDomain* domain);
    SHARD_API int Shard_DestroyDomain(shard::ApplicationDomain* domain);
    SHARD_API shard::VirtualMachine* Shard_GetVirtualMachine(shard::ApplicationDomain* domain);
    SHARD_API shard::GarbageCollector* Shard_GetGarbageCollector(shard::ApplicationDomain* domain);
    SHARD_API shard::ProgramVirtualImage* Shard_GetProgram(shard::ApplicationDomain* domain);
    SHARD_API shard::MethodSymbol* Shard_GetEntryPointMethod(shard::ApplicationDomain* domain);

    SHARD_API int Shard_SetScriptArguments(shard::ApplicationDomain* domain, const wchar_t* const* args, std::size_t count);
    SHARD_API std::size_t Shard_GetScriptArgumentCount(shard::ApplicationDomain* domain);
    SHARD_API int Shard_GetScriptArgument(shard::ApplicationDomain* domain, std::size_t index, wchar_t* buffer, int bufferLen);

    SHARD_API std::size_t Shard_GetProgramDataSectionSize(shard::ProgramVirtualImage* program);
    SHARD_API int Shard_GetProgramDataSectionByte(shard::ProgramVirtualImage* program, std::size_t index);

    // =========================================================================
    // Virtual Machine API
    // =========================================================================

    SHARD_API int Shard_VMRun(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMAbort(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMTerminateCallStack(shard::VirtualMachine* vm);
    SHARD_API shard::ObjectInstance* Shard_VMInvokeMethod(shard::VirtualMachine* vm, shard::MethodSymbol* method, shard::ObjectInstance** args, int argCount);
    SHARD_API int Shard_VMSetPendingTypeArguments(shard::VirtualMachine* vm, shard::TypeSymbol** typeArgs, std::size_t count);

    SHARD_API int Shard_VMGetUnhandledException(shard::VirtualMachine* vm, shard::ObjectInstance** outException);
    SHARD_API int Shard_VMGetUnhandledExceptionMessage(shard::VirtualMachine* vm, wchar_t* buffer, int bufferLen);
    SHARD_API int Shard_VMGetUnhandledExceptionStackTrace(shard::VirtualMachine* vm, wchar_t* buffer, int bufferLen);
    SHARD_API int Shard_VMGetStackTrace(shard::VirtualMachine* vm, wchar_t* buffer, int bufferLen);
    SHARD_API shard::CallStackFrame* Shard_VMGetCurrentFrame(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMRunInteractive(shard::VirtualMachine* vm, std::size_t* pointer);
    SHARD_API shard::CallStackFrame* Shard_VMPushFrame(shard::VirtualMachine* vm, shard::MethodSymbol* method);
    SHARD_API int Shard_VMPopFrame(shard::VirtualMachine* vm);
    SHARD_API int Shard_VMRaiseException(shard::VirtualMachine* vm, shard::ObjectInstance* exception);
    SHARD_API shard::ObjectInstance* Shard_VMCreateRuntimeException(shard::VirtualMachine* vm, const wchar_t* message, shard::TypeSymbol* type);
    SHARD_API const wchar_t* Shard_GetExceptionMessage(shard::VirtualMachine* vm, shard::ObjectInstance* exception);
    SHARD_API const wchar_t* Shard_GetExceptionStackTrace(shard::VirtualMachine* vm, shard::ObjectInstance* exception);
    SHARD_API shard::ObjectInstance* Shard_VMInstantiateObject(shard::VirtualMachine* vm, shard::TypeSymbol* type, shard::ConstructorSymbol* ctor);
    SHARD_API shard::ObjectInstance* Shard_VMInstantiateDelegate(shard::VirtualMachine* vm, shard::DelegateTypeSymbol* type);

    // =========================================================================
    // Call Stack Frame API
    // =========================================================================

    SHARD_API std::size_t Shard_FrameEvalStackCount(shard::CallStackFrame* frame);
    SHARD_API int Shard_FramePushStack(shard::CallStackFrame* frame, shard::ObjectInstance* value);
    SHARD_API shard::ObjectInstance* Shard_FramePopStack(shard::CallStackFrame* frame);
    SHARD_API shard::ObjectInstance* Shard_FramePeekStack(shard::CallStackFrame* frame);
    SHARD_API shard::ObjectInstance* Shard_FrameGetException(shard::CallStackFrame* frame);
    SHARD_API int Shard_FrameGetInterruptionReason(shard::CallStackFrame* frame);

    // =========================================================================
    // Event Loop API
    // =========================================================================

    SHARD_API int Shard_EventLoopRun(shard::ApplicationDomain* domain);
    SHARD_API int Shard_EventLoopRunOnce(shard::ApplicationDomain* domain);
    SHARD_API int Shard_EventLoopStop(shard::ApplicationDomain* domain);
    SHARD_API int Shard_EventLoopIsAlive(shard::ApplicationDomain* domain);
    SHARD_API int Shard_EventLoopRootTask(shard::ApplicationDomain* domain, shard::ObjectInstance* task);
    SHARD_API int Shard_EventLoopUnrootTask(shard::ApplicationDomain* domain, shard::ObjectInstance* task);
    SHARD_API int Shard_EventLoopIsEmptyOrAllTasksCompleted(shard::ApplicationDomain* domain);

    SHARD_API int Shard_GetTaskState(shard::ObjectInstance* task, shard::FieldSymbol* stateField, int* state);
    SHARD_API int Shard_ResumeContinuation(shard::ApplicationDomain* domain, shard::ObjectInstance* task, shard::FieldSymbol* continuationField, shard::MethodSymbol* moveNextMethod);
    SHARD_API std::size_t Shard_EventLoopGetRootedTaskCount(shard::ApplicationDomain* domain);
    SHARD_API shard::ObjectInstance* Shard_EventLoopGetRootedTask(shard::ApplicationDomain* domain, std::size_t index);

    // =========================================================================
    // Async Task API (C bindings for NativeAsync.hpp helpers)
    // =========================================================================

    typedef shard::ObjectInstance* Shard_AsyncScopeHandle;
    typedef void (*Shard_AsyncWorkCallback)(Shard_AsyncScopeHandle task, void* userData);
    typedef void (*Shard_AsyncCallback)(void* userData);
    typedef void (*Shard_AsyncResultCallback)(shard::ObjectInstance* result, void* userData);

    SHARD_API shard::ObjectInstance* Shard_DoAsync(const shard::CallState* ctx, Shard_AsyncWorkCallback callback, void* userData);
    SHARD_API shard::ObjectInstance* Shard_DoValueTask(const shard::CallState* ctx, shard::TypeSymbol* resultType, Shard_AsyncWorkCallback callback, void* userData);

    SHARD_API shard::ObjectInstance* Shard_CompletedTask(const shard::CallState* ctx);
    SHARD_API shard::ObjectInstance* Shard_FaultedTask(const shard::CallState* ctx, shard::ObjectInstance* exception);
    SHARD_API shard::ObjectInstance* Shard_FaultedTaskWithMessage(const shard::CallState* ctx, const wchar_t* message);

    SHARD_API int Shard_TaskComplete(Shard_AsyncScopeHandle task);
    SHARD_API int Shard_TaskFail(Shard_AsyncScopeHandle task, shard::ObjectInstance* exception);
    SHARD_API int Shard_TaskFailWithMessage(Shard_AsyncScopeHandle task, const wchar_t* message);
    SHARD_API int Shard_TaskSetValueTaskResult(Shard_AsyncScopeHandle task, shard::ObjectInstance* result);

    SHARD_API int Shard_TaskDelay(Shard_AsyncScopeHandle task, std::int64_t milliseconds, Shard_AsyncCallback callback, void* userData);
    SHARD_API int Shard_TaskRunOnThreadPool(Shard_AsyncScopeHandle task, Shard_AsyncCallback workCallback, void* workUserData, Shard_AsyncCallback completeCallback, void* completeUserData);
    SHARD_API int Shard_TaskAwait(Shard_AsyncScopeHandle task, shard::ObjectInstance* awaitable, Shard_AsyncCallback callback, void* userData);
    SHARD_API int Shard_TaskAwaitResult(Shard_AsyncScopeHandle task, shard::ObjectInstance* awaitable, Shard_AsyncResultCallback callback, void* userData);

    SHARD_API shard::ObjectInstance* Shard_CreateNativeContinuation(Shard_AsyncScopeHandle task, Shard_AsyncCallback callback, void* userData);
    SHARD_API int Shard_InvokeNativeContinuationCallback(shard::ObjectInstance* continuation);
    SHARD_API int Shard_SetTaskState(shard::ObjectInstance* task, shard::FieldSymbol* stateField, int state, shard::GarbageCollector* gc);

    // =========================================================================
    // Garbage Collector / Value API
    // =========================================================================

    SHARD_API shard::ObjectInstance* Shard_GCFromInteger(shard::GarbageCollector* gc, std::int64_t value);
    SHARD_API shard::ObjectInstance* Shard_GCFromDouble(shard::GarbageCollector* gc, double value);
    SHARD_API shard::ObjectInstance* Shard_GCFromBool(shard::GarbageCollector* gc, int value);
    SHARD_API shard::ObjectInstance* Shard_GCFromString(shard::GarbageCollector* gc, const wchar_t* value);
    SHARD_API shard::ObjectInstance* Shard_GCFromByte(shard::GarbageCollector* gc, std::uint8_t value);
    SHARD_API shard::ObjectInstance* Shard_GCFromChar(shard::GarbageCollector* gc, wchar_t value);
    SHARD_API shard::ObjectInstance* Shard_GCFromNint(shard::GarbageCollector* gc, std::int64_t value);

    SHARD_API shard::ObjectInstance* Shard_GCFromStringWithTransient(shard::GarbageCollector* gc, const wchar_t* value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromIntegerWithTransient(shard::GarbageCollector* gc, std::int64_t value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromDoubleWithTransient(shard::GarbageCollector* gc, double value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromBoolWithTransient(shard::GarbageCollector* gc, int value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromByteWithTransient(shard::GarbageCollector* gc, std::uint8_t value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromCharWithTransient(shard::GarbageCollector* gc, wchar_t value, int isTransient);
    SHARD_API shard::ObjectInstance* Shard_GCFromNintWithTransient(shard::GarbageCollector* gc, std::int64_t value, int isTransient);

    SHARD_API shard::ObjectInstance* Shard_GCCopyInstance(shard::GarbageCollector* gc, shard::ObjectInstance* instance);
    SHARD_API int Shard_GCTerminateInstance(shard::GarbageCollector* gc, shard::ObjectInstance* instance);
    SHARD_API shard::ObjectInstance* Shard_GCNullInstance(shard::GarbageCollector* gc);
    SHARD_API int Shard_GCCollectInstance(shard::GarbageCollector* gc, shard::ObjectInstance* instance);
    SHARD_API int Shard_GCDestroyInstance(shard::GarbageCollector* gc, shard::ObjectInstance* instance);
    SHARD_API int Shard_GCTerminate(shard::GarbageCollector* gc);
    SHARD_API std::size_t Shard_GCGetHeapSize(shard::GarbageCollector* gc);
    SHARD_API shard::ObjectInstance* Shard_GCFromNintPointer(shard::GarbageCollector* gc, void* value);

    SHARD_API std::int64_t Shard_ReadInteger(shard::ObjectInstance* instance);
    SHARD_API double Shard_ReadDouble(shard::ObjectInstance* instance);
    SHARD_API int Shard_ReadBool(shard::ObjectInstance* instance);
    SHARD_API const wchar_t* Shard_ReadString(shard::ObjectInstance* instance);

    // =========================================================================
    // Object Instance API
    // =========================================================================

    SHARD_API shard::TypeSymbol* Shard_GetObjectType(shard::ObjectInstance* instance);
    SHARD_API std::size_t Shard_GetObjectArrayLength(shard::ObjectInstance* instance);
    SHARD_API int Shard_IsNullInstance(shard::ObjectInstance* instance);
    SHARD_API int Shard_IsObjectInBounds(shard::ObjectInstance* instance, std::size_t index);
    SHARD_API shard::ObjectInstance* Shard_GetObjectFieldBySlot(shard::ObjectInstance* instance, std::uint32_t slot);
    SHARD_API int Shard_SetObjectFieldBySlot(shard::ObjectInstance* instance, std::uint32_t slot, shard::ObjectInstance* value);
    SHARD_API int Shard_ObjectWriteByte(shard::ObjectInstance* instance, std::uint8_t value);
    SHARD_API int Shard_ObjectWriteChar(shard::ObjectInstance* instance, wchar_t value);
    SHARD_API int Shard_ObjectWriteNint(shard::ObjectInstance* instance, std::int64_t value);
    SHARD_API int Shard_ObjectAsByte(shard::ObjectInstance* instance, std::uint8_t* out);
    SHARD_API int Shard_ObjectAsChar(shard::ObjectInstance* instance, wchar_t* out);
    SHARD_API int Shard_ObjectAsNint(shard::ObjectInstance* instance, std::int64_t* out);
    SHARD_API int Shard_ObjectIncrementReference(shard::ObjectInstance* instance);
    SHARD_API int Shard_ObjectDecrementReference(shard::ObjectInstance* instance);
    SHARD_API std::int64_t Shard_ObjectGetReferenceCount(shard::ObjectInstance* instance);
    SHARD_API int Shard_ObjectGetIsTransient(shard::ObjectInstance* instance);
    SHARD_API void* Shard_ObjectGetMemory(shard::ObjectInstance* instance);
    SHARD_API shard::TypeShape* Shard_ObjectGetShape(shard::ObjectInstance* instance);
    SHARD_API int Shard_ObjectReadMemory(shard::ObjectInstance* instance, std::size_t offset, std::size_t size, void* dst);
    SHARD_API int Shard_ObjectWriteMemory(shard::ObjectInstance* instance, std::size_t offset, std::size_t size, const void* src);
    SHARD_API void* Shard_ObjectOffsetMemory(shard::ObjectInstance* instance, std::size_t offset, std::size_t size);
    SHARD_API int Shard_ObjectWriteInteger(shard::ObjectInstance* instance, std::int64_t value);
    SHARD_API int Shard_ObjectWriteDouble(shard::ObjectInstance* instance, double value);
    SHARD_API int Shard_ObjectWriteBool(shard::ObjectInstance* instance, int value);
    SHARD_API int Shard_ObjectWriteString(shard::ObjectInstance* instance, const wchar_t* value);

    // =========================================================================
    // TypeShape API
    // =========================================================================

    SHARD_API shard::TypeSymbol* Shard_GetTypeShapeBaseType(shard::TypeShape* shape);
    SHARD_API std::size_t Shard_GetTypeShapeSize(shard::TypeShape* shape);
    SHARD_API std::size_t Shard_GetTypeShapeSlotCount(shard::TypeShape* shape);
    SHARD_API std::size_t Shard_GetTypeShapeSlotOffset(shard::TypeShape* shape, std::uint32_t slot);
    SHARD_API shard::TypeShape* Shard_GetTypeShapeSlotFieldShape(shard::TypeShape* shape, std::uint32_t slot);
    SHARD_API int Shard_GetTypeShapeIsReferenceType(shard::TypeShape* shape);
    SHARD_API int Shard_GetTypeShapeHasGenericArguments(shard::TypeShape* shape);
    SHARD_API std::size_t Shard_GetTypeShapeGenericArgumentCount(shard::TypeShape* shape);
    SHARD_API shard::TypeSymbol* Shard_GetTypeShapeGenericArgument(shard::TypeShape* shape, std::size_t index);
    SHARD_API shard::TypeShape* Shard_GetObjectTypeShape(shard::ObjectInstance* instance);
    SHARD_API shard::TypeShape* Shard_GetOrCreateTypeShape(shard::CompilationContext* ctx, shard::TypeSymbol* baseType, shard::TypeSymbol** genericArgs, std::size_t genericArgCount);
    SHARD_API shard::TypeShape* Shard_GetTypeShapeForType(shard::CompilationContext* ctx, shard::TypeSymbol* type);

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

    SHARD_API shard::TypeSymbol* Shard_FindTypeByName(shard::CompilationContext* ctx, const wchar_t* name);
    SHARD_API shard::FieldSymbol* Shard_FindFieldByName(shard::CompilationContext* ctx, shard::TypeSymbol* type, const wchar_t* name);
    SHARD_API shard::MethodSymbol* Shard_FindMethodByName(shard::CompilationContext* ctx, shard::TypeSymbol* type, const wchar_t* name, int parameterCount);
    SHARD_API int Shard_AreTypesEqual(shard::TypeSymbol* a, shard::TypeSymbol* b);
    SHARD_API int Shard_IsPrimitiveType(shard::CompilationContext* ctx, shard::TypeSymbol* type);
    SHARD_API int Shard_GetTypeDisplayName(shard::TypeSymbol* type, wchar_t* buffer, int bufferLen);
    SHARD_API int Shard_GetSymbolTableNamespaceCount(shard::CompilationContext* ctx);
    SHARD_API shard::NamespaceSymbol* Shard_GetSymbolTableNamespace(shard::CompilationContext* ctx, int index);
    SHARD_API int Shard_GetSymbolTableMethodCount(shard::CompilationContext* ctx);
    SHARD_API shard::MethodSymbol* Shard_GetSymbolTableMethod(shard::CompilationContext* ctx, int index);
    SHARD_API shard::SyntaxSymbol* Shard_LookupSymbol(shard::CompilationContext* ctx, shard::SyntaxNode* node);
    SHARD_API shard::SyntaxNode* Shard_LookupNode(shard::CompilationContext* ctx, shard::SyntaxSymbol* symbol);
    SHARD_API int Shard_MarkAllSymbolsReady(shard::CompilationContext* ctx);

    enum class ShardStandardInterfaceKind : int
    {
        Printable = 0,
        Disposable = 1,
        Enumerable = 2,
        Throwable = 3,
        AsyncState = 4,
        Awaitable = 5,
        Awaiter = 6,
        Enumerator = 7
    };

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
    // Symbol Metadata API
    // =========================================================================

    SHARD_API const wchar_t* Shard_GetSymbolFullName(shard::SyntaxSymbol* symbol);
    SHARD_API int Shard_GetSymbolKind(shard::SyntaxSymbol* symbol);
    SHARD_API shard::SyntaxSymbol* Shard_GetSymbolParent(shard::SyntaxSymbol* symbol);
    SHARD_API int Shard_GetSymbolAnalysisState(shard::SyntaxSymbol* symbol);
    SHARD_API int Shard_GetSymbolLinking(shard::SyntaxSymbol* symbol);

    SHARD_API int Shard_GetTypeConstructorCount(shard::CompilationContext* ctx, shard::TypeSymbol* type);
    SHARD_API shard::MethodSymbol* Shard_GetTypeConstructor(shard::CompilationContext* ctx, shard::TypeSymbol* type, int index);
    SHARD_API int Shard_GetTypePropertyCount(shard::CompilationContext* ctx, shard::TypeSymbol* type);
    SHARD_API shard::PropertySymbol* Shard_GetTypeProperty(shard::CompilationContext* ctx, shard::TypeSymbol* type, int index);
    SHARD_API int Shard_GetTypeIndexatorCount(shard::CompilationContext* ctx, shard::TypeSymbol* type);
    SHARD_API shard::IndexatorSymbol* Shard_GetTypeIndexator(shard::CompilationContext* ctx, shard::TypeSymbol* type, int index);
    SHARD_API int Shard_GetTypeOperatorCount(shard::CompilationContext* ctx, shard::TypeSymbol* type);
    SHARD_API shard::OperatorSymbol* Shard_GetTypeOperator(shard::CompilationContext* ctx, shard::TypeSymbol* type, int index);
    SHARD_API int Shard_GetTypeMemorySize(shard::TypeSymbol* type);
    SHARD_API int Shard_IsTypeReferenceType(shard::TypeSymbol* type);
    SHARD_API int Shard_IsTypeNullable(shard::TypeSymbol* type);

    SHARD_API int Shard_IsMethodAbstract(shard::MethodSymbol* method);
    SHARD_API int Shard_IsMethodAsync(shard::MethodSymbol* method);
    SHARD_API int Shard_SetMethodAsync(shard::MethodSymbol* method, int isAsync);
    SHARD_API int Shard_GetMethodTypeParameterCount(shard::MethodSymbol* method);
    SHARD_API shard::TypeParameterSymbol* Shard_GetMethodTypeParameter(shard::MethodSymbol* method, int index);
    SHARD_API int Shard_GetMethodEvalStackArgumentsCount(shard::MethodSymbol* method);
    SHARD_API int Shard_GetMethodEvalStackVariablesCount(shard::MethodSymbol* method);
    SHARD_API int Shard_GetMethodEvalStackLocalsCount(shard::MethodSymbol* method);

    SHARD_API shard::AccessorSymbol* Shard_GetPropertyGetter(shard::PropertySymbol* property);
    SHARD_API shard::AccessorSymbol* Shard_GetPropertySetter(shard::PropertySymbol* property);
    SHARD_API shard::FieldSymbol* Shard_GetPropertyBackingField(shard::PropertySymbol* property);
    SHARD_API int Shard_GetIndexatorParameterCount(shard::IndexatorSymbol* indexator);
    SHARD_API shard::ParameterSymbol* Shard_GetIndexatorParameter(shard::IndexatorSymbol* indexator, int index);
    SHARD_API int Shard_IsParameterOptional(shard::ParameterSymbol* parameter);
    SHARD_API shard::ExpressionSyntax* Shard_GetParameterDefaultValue(shard::ParameterSymbol* parameter);
    SHARD_API int Shard_GetParameterSlotIndex(shard::ParameterSymbol* parameter);
    SHARD_API int Shard_GetFieldOffset(shard::FieldSymbol* field);
    SHARD_API int Shard_GetFieldSlotIndex(shard::FieldSymbol* field);
    SHARD_API int Shard_IsFieldEnumValue(shard::FieldSymbol* field);
    SHARD_API std::int64_t Shard_GetFieldEnumValue(shard::FieldSymbol* field);

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
    SHARD_API shard::ObjectInstance* Shard_GCAllocateInstanceFromShape(shard::GarbageCollector* gc, shard::TypeShape* shape);
    SHARD_API shard::ObjectInstance* Shard_GCAllocateGeneric(shard::GarbageCollector* gc, shard::TypeSymbol* baseType, shard::TypeSymbol** genericArgs, std::size_t genericArgCount);

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

    // Destroy standalone syntax nodes / symbols created by the builder API but never added to a context.
    // Nodes/symbols owned by a CompilationContext or SymbolTable must NOT be passed to these functions.
    SHARD_API int Shard_DestroySyntaxNode(shard::SyntaxNode* node);
    SHARD_API int Shard_DestroySymbol(shard::SyntaxSymbol* symbol);

    // =========================================================================
    // Library Metadata API
    // =========================================================================

    SHARD_API int Shard_ReadLibraryMetadata(const wchar_t* path, shard::ShardLibMetadata* out);
    SHARD_API int Shard_GetLibraryDependencyCount(const shard::ShardLibMetadata* metadata);
    SHARD_API const wchar_t* Shard_GetLibraryDependencyName(const shard::ShardLibMetadata* metadata, int index);
    SHARD_API const wchar_t* Shard_GetLibraryDependencyVersionExpression(const shard::ShardLibMetadata* metadata, int index);
    SHARD_API const wchar_t* Shard_GetLibraryName(const shard::ShardLibMetadata* metadata);
    SHARD_API const wchar_t* Shard_GetLibraryDescription(const shard::ShardLibMetadata* metadata);
    SHARD_API const wchar_t* Shard_GetLibraryVersion(const shard::ShardLibMetadata* metadata);

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
    // 'name' is ignored for binary compatibility; constructors are always named 'init'.
    SHARD_API shard::ConstructorDeclarationSyntax* Shard_CreateConstructorDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_SetMethodReturnType(shard::MethodDeclarationSyntax* method, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetMethodParametersList(shard::MethodDeclarationSyntax* method, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetMethodBody(shard::MethodDeclarationSyntax* method, shard::StatementsBlockSyntax* body);
    SHARD_API int Shard_SetConstructorParametersList(shard::ConstructorDeclarationSyntax* ctor, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetConstructorBody(shard::ConstructorDeclarationSyntax* ctor, shard::StatementsBlockSyntax* body);

    SHARD_API shard::PropertyDeclarationSyntax* Shard_CreatePropertyDeclaration(shard::SyntaxNode* parent, const wchar_t* name, shard::TypeSyntax* type);
    SHARD_API shard::AccessorDeclarationSyntax* Shard_CreateAccessorDeclaration(shard::SyntaxNode* parent, int keywordTokenType);
    SHARD_API int Shard_PropertyDeclarationAddGetter(shard::PropertyDeclarationSyntax* property, shard::AccessorDeclarationSyntax* getter);
    SHARD_API int Shard_PropertyDeclarationAddSetter(shard::PropertyDeclarationSyntax* property, shard::AccessorDeclarationSyntax* setter);
    SHARD_API int Shard_SetAccessorBody(shard::AccessorDeclarationSyntax* accessor, shard::StatementsBlockSyntax* body);

    SHARD_API shard::InterfaceDeclarationSyntax* Shard_CreateInterfaceDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API shard::EnumDeclarationSyntax* Shard_CreateEnumDeclaration(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_AddEnumField(shard::EnumDeclarationSyntax* enumDecl, shard::EnumFieldDeclarationSyntax* field);
    SHARD_API shard::EnumFieldDeclarationSyntax* Shard_CreateEnumFieldDeclaration(shard::SyntaxNode* parent, const wchar_t* name, shard::ExpressionSyntax* value);
    SHARD_API shard::OperatorDeclarationSyntax* Shard_CreateOperatorDeclaration(shard::SyntaxNode* parent, int operatorTokenType, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetOperatorParametersList(shard::OperatorDeclarationSyntax* op, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetOperatorBody(shard::OperatorDeclarationSyntax* op, shard::StatementsBlockSyntax* body);
    SHARD_API shard::IndexatorDeclarationSyntax* Shard_CreateIndexatorDeclaration(shard::SyntaxNode* parent, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetIndexatorParametersList(shard::IndexatorDeclarationSyntax* indexer, shard::ParametersListSyntax* parameters);
    SHARD_API int Shard_SetIndexatorBody(shard::IndexatorDeclarationSyntax* indexer, shard::StatementsBlockSyntax* body);
    SHARD_API shard::DelegateDeclarationSyntax* Shard_CreateDelegateDeclaration(shard::SyntaxNode* parent, const wchar_t* name, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetDelegateParametersList(shard::DelegateDeclarationSyntax* delegate, shard::ParametersListSyntax* parameters);

    SHARD_API shard::AttributeSyntax* Shard_CreateAttribute(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_AddAttribute(shard::MemberDeclarationSyntax* member, shard::AttributeSyntax* attribute);
    SHARD_API shard::TypeParametersListSyntax* Shard_CreateTypeParametersList(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddTypeParameter(shard::TypeParametersListSyntax* list, const wchar_t* name);
    SHARD_API int Shard_SetMethodTypeParametersList(shard::MethodDeclarationSyntax* method, shard::TypeParametersListSyntax* list);
    SHARD_API int Shard_SetClassTypeParametersList(shard::ClassDeclarationSyntax* cls, shard::TypeParametersListSyntax* list);

    SHARD_API shard::WhereClauseSyntax* Shard_CreateWhereClause(shard::SyntaxNode* parent, const wchar_t* typeParameterName);
    SHARD_API int Shard_AddWhereClauseConstraint(shard::WhereClauseSyntax* whereClause, shard::TypeSyntax* constraintType);
    SHARD_API int Shard_AddMemberWhereClause(shard::MemberDeclarationSyntax* member, shard::WhereClauseSyntax* whereClause);

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

    SHARD_API shard::QualifiedNameTypeSyntax* Shard_CreateQualifiedNameType(shard::SyntaxNode* parent, shard::TypeSyntax* left, const wchar_t* right);
    SHARD_API shard::DelegateTypeSyntax* Shard_CreateDelegateType(shard::SyntaxNode* parent, shard::TypeSyntax* returnType);
    SHARD_API int Shard_SetDelegateTypeParametersList(shard::DelegateTypeSyntax* type, shard::ParametersListSyntax* parameters);

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

    SHARD_API shard::ForStatementSyntax* Shard_CreateForStatement(shard::SyntaxNode* parent, shard::StatementSyntax* init, shard::ExpressionSyntax* condition, shard::StatementSyntax* after, shard::StatementsBlockSyntax* body);
    SHARD_API shard::ForInStatementSyntax* Shard_CreateForInStatement(shard::SyntaxNode* parent, const wchar_t* variableName, shard::ExpressionSyntax* range, shard::StatementsBlockSyntax* body);
    SHARD_API shard::UntilStatementSyntax* Shard_CreateUntilStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::StatementsBlockSyntax* body);
    SHARD_API shard::IfStatementSyntax* Shard_CreateIfStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::StatementsBlockSyntax* thenBody);
    SHARD_API int Shard_IfStatementSetElse(shard::IfStatementSyntax* ifStmt, shard::ElseStatementSyntax* elseBody);
    SHARD_API shard::UnlessStatementSyntax* Shard_CreateUnlessStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::StatementsBlockSyntax* body);
    SHARD_API shard::ElseStatementSyntax* Shard_CreateElseStatement(shard::SyntaxNode* parent, shard::StatementsBlockSyntax* body);
    SHARD_API shard::SwitchStatementSyntax* Shard_CreateSwitchStatement(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API int Shard_AddSwitchCase(shard::SwitchStatementSyntax* switchStmt, shard::SwitchCaseClauseSyntax* clause);
    SHARD_API shard::SwitchCaseClauseSyntax* Shard_CreateSwitchCaseClause(shard::SyntaxNode* parent, shard::ExpressionSyntax* pattern, shard::StatementsBlockSyntax* body);
    SHARD_API shard::ConditionalClauseSyntax* Shard_CreateConditionalClause(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::StatementSyntax* statement);

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

    SHARD_API shard::TernaryExpressionSyntax* Shard_CreateTernaryExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::ExpressionSyntax* trueExpr, shard::ExpressionSyntax* falseExpr);
    SHARD_API shard::IfExpressionSyntax* Shard_CreateIfExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* condition, shard::ExpressionSyntax* thenExpr, shard::ExpressionSyntax* elseExpr);
    SHARD_API shard::SwitchExpressionSyntax* Shard_CreateSwitchExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API int Shard_AddSwitchExpressionArm(shard::SwitchExpressionSyntax* switchExpr, shard::SwitchArmSyntax* arm);
    SHARD_API shard::SwitchArmSyntax* Shard_CreateSwitchArm(shard::SyntaxNode* parent, shard::ExpressionSyntax* pattern, shard::ExpressionSyntax* value);
    SHARD_API shard::CastExpressionSyntax* Shard_CreateCastExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression, shard::TypeSyntax* targetType);
    SHARD_API shard::IsExpressionSyntax* Shard_CreateIsExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression, shard::TypeSyntax* type);
    SHARD_API shard::IsPatternSyntax* Shard_CreateIsPattern(shard::SyntaxNode* parent, shard::TypeSyntax* type);
    SHARD_API shard::IndexatorExpressionSyntax* Shard_CreateIndexatorExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* target, shard::ExpressionSyntax* index);
    SHARD_API shard::LambdaExpressionSyntax* Shard_CreateLambdaExpression(shard::SyntaxNode* parent, shard::ParametersListSyntax* parameters, shard::TypeSyntax* returnType, shard::StatementsBlockSyntax* body);
    SHARD_API shard::AwaitExpressionSyntax* Shard_CreateAwaitExpression(shard::SyntaxNode* parent, shard::ExpressionSyntax* expression);
    SHARD_API shard::TypeExpressionSyntax* Shard_CreateTypeExpression(shard::SyntaxNode* parent, shard::TypeSyntax* type);

    SHARD_API shard::ArgumentsListSyntax* Shard_CreateArgumentsList(shard::SyntaxNode* parent);
    SHARD_API int Shard_AddArgument(shard::ArgumentsListSyntax* list, shard::ExpressionSyntax* expression);

    SHARD_API int Shard_AddCompilationUnitUsing(shard::CompilationUnitSyntax* unit, shard::UsingDirectiveSyntax* usingDirective);
    SHARD_API shard::UsingDirectiveSyntax* Shard_CreateUsingDirective(shard::SyntaxNode* parent, const wchar_t* name);
    SHARD_API int Shard_AddTypeBaseInterface(shard::TypeDeclarationSyntax* type, shard::TypeSyntax* interfaceType);
    SHARD_API int Shard_SetStatementsBlockExpressionBody(shard::StatementsBlockSyntax* block, int isExpressionBody);

    // =========================================================================
    // Symbol Builder API
    // =========================================================================

    SHARD_API shard::SymbolTable* Shard_GetSymbolTable(shard::CompilationContext* ctx);
    SHARD_API shard::TypeSymbol* Shard_GetPrimitiveType(shard::CompilationContext* ctx, int primitiveKind);
    SHARD_API shard::NamespaceSymbol* Shard_CreateNamespaceSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name);
    SHARD_API shard::ClassSymbol* Shard_CreateClassSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name);
    SHARD_API shard::MethodSymbol* Shard_CreateMethodSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* returnType, int isStatic, int accessibility);
    SHARD_API shard::MethodSymbol* Shard_CreateNamespaceMethodSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parentNamespace, const wchar_t* name, shard::TypeSymbol* returnType, int isStatic, int accessibility);
    SHARD_API shard::ConstructorSymbol* Shard_CreateConstructorSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, int accessibility);
    SHARD_API shard::ParameterSymbol* Shard_CreateParameterSymbol(shard::CompilationContext* ctx, const wchar_t* name, shard::TypeSymbol* type);
    SHARD_API int Shard_AddMethodParameter(shard::MethodSymbol* method, shard::ParameterSymbol* parameter);
    SHARD_API shard::FieldSymbol* Shard_CreateFieldSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* type, int isStatic, int accessibility);
    SHARD_API int Shard_SetSymbolAccesibility(shard::SyntaxSymbol* symbol, int accessibility);
    SHARD_API int Shard_SetSymbolLinking(shard::SyntaxSymbol* symbol, int linking);

    // =========================================================================
    // Native callback binding helpers (used by language bindings such as Rust)
    // =========================================================================

    SHARD_API int Shard_SetMethodCallback(shard::MethodSymbol* method, shard::MethodSymbolDelegate callback);
    SHARD_API int Shard_SetConstructorCallback(shard::ConstructorSymbol* ctor, shard::MethodSymbolDelegate callback);
    SHARD_API int Shard_SetAccessorCallback(shard::AccessorSymbol* accessor, shard::MethodSymbolDelegate callback);

    SHARD_API int Shard_SetMethodManagedCallStateCallback(shard::MethodSymbol* method, shard::ObjectInstance* (*callback)(const shard::CallState* state, void* userData), void* userData);

    typedef shard::ObjectInstance* (*ShardManagedMethodCallback)(
        shard::MethodSymbol* method,
        shard::ObjectInstance** args,
        int argsCount,
        void* userData,
        shard::GarbageCollector* collector);

    SHARD_API int Shard_SetMethodManagedCallback(shard::MethodSymbol* method, ShardManagedMethodCallback callback, void* userData);

    SHARD_API shard::PropertySymbol* Shard_CreatePropertySymbol(
        shard::CompilationContext* ctx,
        shard::TypeSymbol* parentType,
        const wchar_t* name,
        shard::TypeSymbol* type,
        int isStatic,
        int accessibility);

    SHARD_API shard::AccessorSymbol* Shard_PropertyAddGetter(shard::CompilationContext* ctx, shard::PropertySymbol* property);
    SHARD_API shard::AccessorSymbol* Shard_PropertyAddSetter(shard::CompilationContext* ctx, shard::PropertySymbol* property);

    SHARD_API shard::InterfaceSymbol* Shard_CreateInterfaceSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name, int accessibility);
    SHARD_API int Shard_TypeAddInterface(shard::TypeSymbol* type, shard::InterfaceSymbol* interfaceType);
    SHARD_API int Shard_TypeAddGenericInterface(shard::TypeSymbol* type, shard::GenericTypeSymbol* interfaceType);
    SHARD_API int Shard_ClassSetInterfaceMethodImplementation(shard::ClassSymbol* classType, shard::MethodSymbol* interfaceMethod, shard::MethodSymbol* implementationMethod);
    SHARD_API shard::StructSymbol* Shard_CreateStructSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name);
    SHARD_API shard::EnumSymbol* Shard_CreateEnumSymbol(shard::CompilationContext* ctx, shard::NamespaceSymbol* parent, const wchar_t* name, int isFlags);
    SHARD_API shard::FieldSymbol* Shard_AddEnumLiteral(shard::CompilationContext* ctx, shard::EnumSymbol* enumType, const wchar_t* name, std::int64_t value);
    SHARD_API shard::OperatorSymbol* Shard_CreateOperatorSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* returnType, int operatorTokenType, int accessibility);
    SHARD_API shard::TypeParameterSymbol* Shard_CreateTypeParameterSymbol(shard::CompilationContext* ctx, shard::SyntaxSymbol* parent, const wchar_t* name);
    SHARD_API int Shard_AddTypeParameterConstraint(shard::TypeParameterSymbol* typeParam, shard::TypeSymbol* constraint);
    SHARD_API std::size_t Shard_GetTypeParameterConstraintCount(shard::TypeParameterSymbol* typeParam);
    SHARD_API shard::TypeSymbol* Shard_GetTypeParameterConstraint(shard::TypeParameterSymbol* typeParam, std::size_t index);
    SHARD_API shard::ArrayTypeSymbol* Shard_CreateArrayTypeSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* elementType);
    SHARD_API shard::GenericTypeSymbol* Shard_CreateGenericTypeSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* underlyingType, shard::TypeSymbol** typeArgs, std::size_t typeArgCount);
    SHARD_API shard::TypeSymbol* Shard_GetGenericTypeUnderlyingType(shard::GenericTypeSymbol* generic);
    SHARD_API shard::IndexatorSymbol* Shard_CreateIndexatorSymbol(shard::CompilationContext* ctx, shard::TypeSymbol* parentType, const wchar_t* name, shard::TypeSymbol* returnType, int accessibility);
    SHARD_API int Shard_AddIndexatorParameter(shard::IndexatorSymbol* indexator, shard::ParameterSymbol* parameter);
    SHARD_API shard::AccessorSymbol* Shard_IndexatorAddGetter(shard::CompilationContext* ctx, shard::IndexatorSymbol* indexator);
    SHARD_API shard::AccessorSymbol* Shard_IndexatorAddSetter(shard::CompilationContext* ctx, shard::IndexatorSymbol* indexator);

    // =========================================================================
    // CallState / argument accessors
    // =========================================================================

    SHARD_API std::size_t Shard_CallStateArgCount(const shard::CallState* state);
    SHARD_API shard::ObjectInstance* Shard_CallStateArg(const shard::CallState* state, std::size_t index);
    SHARD_API shard::GarbageCollector* Shard_CallStateCollector(const shard::CallState* state);
    SHARD_API shard::MethodSymbol* Shard_CallStateMethod(const shard::CallState* state);
    SHARD_API shard::CallStackFrame* Shard_CallStateFrame(const shard::CallState* state);

    // =========================================================================
    // String length helper
    // =========================================================================

    SHARD_API std::int64_t Shard_ReadStringLength(shard::ObjectInstance* instance);
}
