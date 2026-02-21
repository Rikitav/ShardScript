#include <shard/parsing/SyntaxTree.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/parsing/lexical/LexicalAnalyzer.h>
#include <shard/parsing/lexical/reading/FileReader.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/SourceParser.h>
#include <shard/parsing/LayoutGenerator.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/ProgramDisassembler.h>

#include <shard/runtime/framework/FrameworkLoader.h>

#include <shard/compilation/AbstractEmiter.h>
#include <shard/compilation/ProgramVirtualImage.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <Windows.h>
#include <dbghelp.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dbghelp.lib")

#include "InteractiveConsole.h"
#include "utilities/InterpreterUtilities.h"

using namespace shard;
namespace fs = std::filesystem;

const fs::path stdlibFilename = "ShardScript.Framework.dll";

static void SigIntHandler(int signal)
{
	//AbstractInterpreter::TerminateCallStack();
	GarbageCollector::Terminate();
	exit(SIGINT);
}

static LONG WINAPI UnhandledExceptionFilterFunction(PEXCEPTION_POINTERS exception)
{
    PEXCEPTION_RECORD exceptionRecord = exception->ExceptionRecord;
    PCONTEXT contextRecord = exception->ContextRecord;

    std::cout << "\n========== UNHANDLED EXCEPTION ==========\n" << std::endl;
    std::cout << "Exception Code: 0x" << std::hex << exceptionRecord->ExceptionCode;

    // Расшифровываем код исключения
    switch (exceptionRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:            std::cout << " (ACCESS VIOLATION)" << std::endl; break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:       std::cout << " (ARRAY BOUNDS EXCEEDED)" << std::endl; break;
        case EXCEPTION_BREAKPOINT:                  std::cout << " (BREAKPOINT)" << std::endl; break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:       std::cout << " (DATATYPE MISALIGNMENT)" << std::endl; break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:        std::cout << " (FLOAT DENORMAL OPERAND)" << std::endl; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:          std::cout << " (FLOAT DIVIDE BY ZERO)" << std::endl; break;
        case EXCEPTION_FLT_INEXACT_RESULT:          std::cout << " (FLOAT INEXACT RESULT)" << std::endl; break;
        case EXCEPTION_FLT_INVALID_OPERATION:       std::cout << " (FLOAT INVALID OPERATION)" << std::endl; break;
        case EXCEPTION_FLT_OVERFLOW:                std::cout << " (FLOAT OVERFLOW)" << std::endl; break;
        case EXCEPTION_FLT_STACK_CHECK:             std::cout << " (FLOAT STACK CHECK)" << std::endl; break;
        case EXCEPTION_FLT_UNDERFLOW:               std::cout << " (FLOAT UNDERFLOW)" << std::endl; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:         std::cout << " (ILLEGAL INSTRUCTION)" << std::endl; break;
        case EXCEPTION_IN_PAGE_ERROR:               std::cout << " (IN PAGE ERROR)" << std::endl; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:          std::cout << " (INTEGER DIVIDE BY ZERO)" << std::endl; break;
        case EXCEPTION_INT_OVERFLOW:                std::cout << " (INTEGER OVERFLOW)" << std::endl; break;
        case EXCEPTION_INVALID_DISPOSITION:         std::cout << " (INVALID DISPOSITION)" << std::endl; break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:    std::cout << " (NONCONTINUABLE EXCEPTION)" << std::endl; break;
        case EXCEPTION_PRIV_INSTRUCTION:            std::cout << " (PRIVILEGED INSTRUCTION)" << std::endl; break;
        case EXCEPTION_SINGLE_STEP:                 std::cout << " (SINGLE STEP)" << std::endl; break;
        case EXCEPTION_STACK_OVERFLOW:              std::cout << " (STACK OVERFLOW)" << std::endl; break;
        default:                                    std::cout << " (UNKNOWN EXCEPTION)" << std::endl; break;
    }

    if (exceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && exceptionRecord->NumberParameters >= 2)
    {
        std::cout << "Access Violation Type: ";
        switch (exceptionRecord->ExceptionInformation[0])
        {
            case 0:  std::cout << "Read" << std::endl; break;
            case 1:  std::cout << "Write" << std::endl; break;
            case 8:  std::cout << "DEP violation" << std::endl; break;
            default: std::cout << "Unknown (0x" << exceptionRecord->ExceptionInformation[0] << ")" << std::endl; break;
        }
        
        std::cout << "Access Address: 0x" << exceptionRecord->ExceptionInformation[1] << std::endl;
    }

    std::cout << "Exception Address: 0x" << exceptionRecord->ExceptionAddress << std::endl;
    std::cout << "Exception Flags: 0x" << exceptionRecord->ExceptionFlags << std::endl;

    if (exceptionRecord->NumberParameters > 0)
    {
        std::cout << "Number of Parameters: " << exceptionRecord->NumberParameters << std::endl;
        std::cout << "Parameters: ";

        for (ULONG i = 0; i < min(exceptionRecord->NumberParameters, 15); i++)
            std::cout << "0x" << exceptionRecord->ExceptionInformation[i] << ", ";

        std::cout << std::endl;
    }
    
    HMODULE hModule;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)exceptionRecord->ExceptionAddress,
        &hModule))
    {
        char modulePath[MAX_PATH];
        if (GetModuleFileNameA(hModule, modulePath, MAX_PATH))
        {
            std::cout << "\nModule: " << modulePath << std::endl;
        }
    }

    std::cout << "\n========== END OF EXCEPTION REPORT ==========" << std::endl;

    HANDLE logFile = CreateFileA("exception.log",
        FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (logFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(logFile);
    }

	MINIDUMP_EXCEPTION_INFORMATION minidumpInfo{};
    minidumpInfo.ThreadId = GetCurrentThreadId();
    minidumpInfo.ExceptionPointers = exception;
    minidumpInfo.ClientPointers = FALSE;

    HANDLE dumpFile = CreateFileA("crash.dmp",
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (dumpFile != INVALID_HANDLE_VALUE)
    {
        MiniDumpWriteDump(GetCurrentProcess(),
            GetCurrentProcessId(), dumpFile, static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData), &minidumpInfo, NULL, NULL);

        CloseHandle(dumpFile);
        std::cout << std::endl << "Mini-dump saved to crash.dmp" << std::endl;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static bool CheckFilesExisting()
{
	bool anyUnrealFiles = false;
	for (const std::wstring& file : shard::ConsoleArguments::FilesToCompile)
	{
		if (!fs::exists(file))
		{
			anyUnrealFiles = true;
			std::wcout << L"'" << file << L"' doesn't exists";
		}
	}

	return !anyUnrealFiles;
}

static void LoadLibrariesFromDirectoryPath(fs::path path)
{
	for (const fs::directory_entry& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filename = entry.path().filename();
		if (!filename.string().ends_with(".dll"))
			continue;

		FrameworkLoader::AddLib(entry.path().wstring());
	}
}

static fs::path GetCurrentDirectoryPath()
{
	WCHAR pathBuffer[MAX_PATH];
	GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);
	return fs::path(pathBuffer).parent_path();
}

static fs::path GetWorkingDirectoryPath()
{
	WCHAR pathBuffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, pathBuffer);
	return fs::path(pathBuffer);
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		setlocale(LC_ALL, "");
		signal(SIGINT, SigIntHandler);
		SetUnhandledExceptionFilter(UnhandledExceptionFilterFunction);
		ShardUtilities::ParseArguments(argc, argv);

		if (shard::ConsoleArguments::ShowHelp)
		{
			std::wstring version = ShardUtilities::GetFileVersion();
			std::wcout << "ShardLang interpreter v" << version;
			return 0;
		}

		if (shard::ConsoleArguments::AssociateScriptFile)
		{
			ShardUtilities::AssociateRegistry();
			std::wcout << "File association successsfuly installed" << std::endl;
			return 0;
		}

		if (!CheckFilesExisting())
			return 1;

		DiagnosticsContext diagnostics;
		SyntaxTree syntaxTree;
		SemanticModel semanticModel(syntaxTree);

		fs::path currentDirectory = GetCurrentDirectoryPath();
		if (!shard::ConsoleArguments::ExcludeStd)
		{
			fs::path stdlibFilepath = currentDirectory / stdlibFilename;
			if (!fs::exists(stdlibFilepath))
			{
				std::wcout << "'" << stdlibFilename << "' not found! use '--no-std' flag to disable standart library loading requirement" << std::endl;
				return 1;
			}

			FrameworkLoader::AddLib(stdlibFilepath.wstring());
		}

		FrameworkLoader::Load(semanticModel, diagnostics);

		fs::path workingDirectory = GetWorkingDirectoryPath();
		if (!fs::exists(workingDirectory))
		{
			std::wcout << "Could not resolve current working directory!" << std::endl;
			return 1;
		}

		SourceParser parser(diagnostics);
		for (const std::wstring& file : shard::ConsoleArguments::FilesToCompile)
		{
			FileReader reader(file);
			LexicalAnalyzer lexer(reader);
			parser.FromSourceProvider(syntaxTree, lexer);
		}

		SemanticAnalyzer semanticAnalyzer(diagnostics);
		semanticAnalyzer.Analyze(syntaxTree, semanticModel);

		if (diagnostics.AnyError)
		{
			std::wcout << L"=== Diagnostics output ===" << std::endl;
			diagnostics.WriteDiagnostics(std::wcout);
			return 1;
		}

		LayoutGenerator layoutGenerator(diagnostics);
		layoutGenerator.Generate(semanticModel);

		ProgramVirtualImage program;
		AbstractEmiter emiter(program, semanticModel, diagnostics);

		emiter.VisitSyntaxTree(syntaxTree);
		if (ConsoleArguments::RunProgram)
			emiter.SetEntryPoint();

		if (diagnostics.AnyError)
		{
			std::wcout << L"=== Diagnostics output ===" << std::endl;
			diagnostics.WriteDiagnostics(std::wcout);
			return 1;
		}

		if (ConsoleArguments::UseInteractive)
		{
			InteractiveConsole repl(syntaxTree, semanticModel, diagnostics);
			repl.Run();
			return 0;
		}

		if (ConsoleArguments::ShowDecompile)
		{
			ProgramDisassembler disassembler;
			disassembler.Disassemble(std::wcout, program);
		}

		if (ConsoleArguments::RunProgram)
		{
			VirtualMachine virtualMachine(program);
			virtualMachine.Run();
		}
	}
	catch (const std::runtime_error& err)
	{
		std::cout << "CRITICAL ERROR : " << err.what() << std::endl;
	}

	GarbageCollector::Terminate();
	FrameworkLoader::Destroy();
}