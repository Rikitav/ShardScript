#include <shard/runtime/VirtualMachine.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/LayoutGenerator.hpp>
#include <shard/runtime/framework/FrameworkLoader.hpp>
#include <shard/compilation/AbstractEmiter.hpp>
#include <sstream>
#include <algorithm>

#define SHARD_EXPORT extern "C" __declspec(dllexport)

using namespace shard;

SHARD_EXPORT ProgramVirtualImage* Program_Create()
{
    ProgramVirtualImage* program = new ProgramVirtualImage();
    return program;
}

SHARD_EXPORT void Program_Destroy(ProgramVirtualImage* program)
{
    delete program;
}

SHARD_EXPORT VirtualMachine* VirtualMachine_Create(ProgramVirtualImage* programImagePtr)
{
    if (programImagePtr == nullptr)
        return nullptr;

    ProgramVirtualImage* program = static_cast<ProgramVirtualImage*>(programImagePtr);
    VirtualMachine* virtualMachine = new VirtualMachine(*program);

    return virtualMachine;
}

SHARD_EXPORT void VirtualMachine_Destroy(VirtualMachine* virtualMachinePtr)
{
    if (virtualMachinePtr == nullptr)
        return;

    delete virtualMachinePtr;
}

SHARD_EXPORT void VirtualMachine_Run(VirtualMachine* virtualMachinePtr)
{
    if (virtualMachinePtr == nullptr)
        return;

    VirtualMachine* virtualMachine = static_cast<VirtualMachine*>(virtualMachinePtr);
    virtualMachine->Run();
}

SHARD_EXPORT void AddLibrary(const wchar_t* path)
{
    if (path != nullptr)
        FrameworkLoader::AddLib(std::wstring(path));
}

struct CompilationContext
{
    DiagnosticsContext Diagnostics;
    SyntaxTree Tree;
    SemanticModel Model;

    CompilationContext() : Model(Tree) {}
};

SHARD_EXPORT CompilationContext* CompilationContext_Create()
{
    return new CompilationContext();
}

SHARD_EXPORT void CompilationContext_Destroy(CompilationContext* ctx)
{
    delete ctx;
}

SHARD_EXPORT void LoadFramework(CompilationContext* ctx)
{
    if (ctx)
        FrameworkLoader::Load(ctx->Model, ctx->Diagnostics);
}

SHARD_EXPORT int GetDiagnostics(CompilationContext* ctx, wchar_t* buffer, int bufferLen)
{
    if (!ctx) return 0;
    std::wstringstream ss;
    ctx->Diagnostics.WriteDiagnostics(ss);
    std::wstring str = ss.str();

    if (buffer && bufferLen > 0) {
        size_t copyLen = (std::min)((size_t)bufferLen - 1, str.length());
        wcsncpy_s(buffer, bufferLen, str.c_str(), copyLen);
    }
    return (int)str.length();
}

SHARD_EXPORT int CompileCode(ProgramVirtualImage* programPtr, CompilationContext* context, const wchar_t* code)
{
    if (programPtr == nullptr || context == nullptr || code == nullptr)
        return -1;

    std::wstring sourceCode(code);
    StringStreamReader reader(L"Host", sourceCode);
    LexicalAnalyzer lexer(reader);
    SourceParser parser(context->Diagnostics);
    parser.FromSourceProvider(context->Tree, lexer);

    SemanticAnalyzer semanticAnalyzer(context->Diagnostics);
    semanticAnalyzer.Analyze(context->Tree, context->Model);

    if (context->Diagnostics.AnyError)
        return 1;

    LayoutGenerator layoutGenerator(context->Diagnostics);
    layoutGenerator.Generate(context->Model);

    AbstractEmiter emiter(*programPtr, context->Model, context->Diagnostics);
    emiter.VisitSyntaxTree(context->Tree);
    emiter.SetEntryPoint();

    if (context->Diagnostics.AnyError)
        return 2;

    return 0;
}