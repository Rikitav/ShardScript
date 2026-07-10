#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <string>
#include <stdexcept>
#include <exception>
#include <unordered_map>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <ios>
#include <vector>

#include <ShardScript.hpp>
#include <InteractiveConsole.hpp>
#include <utilities/InterpreterUtilities.hpp>

// Platform-specific headers
#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#elif defined(__linux__)
	#include <unistd.h>
	#include <limits.h>
#elif defined(__APPLE__)
	#include <mach-o/dyld.h>
	#include <climits>
#endif

using namespace shard;
namespace fs = std::filesystem;

// ANSI color escape sequences
static const wchar_t* CReset   = L"\x1B[0m";
static const wchar_t* CBold    = L"\x1B[1m";
static const wchar_t* CRed     = L"\x1B[91m";
static const wchar_t* CYellow  = L"\x1B[93m";
static const wchar_t* CBlue    = L"\x1B[94m";
static const wchar_t* CCyan    = L"\x1B[96m";
static const wchar_t* CGray    = L"\x1B[90m";
static const wchar_t* CWhite   = L"\x1B[97m";

static void EnableTerminalColors()
{
#if defined(_WIN32)
	auto enableFor = [](DWORD handle)
	{
		HANDLE h = GetStdHandle(handle);
		if (h == INVALID_HANDLE_VALUE)
			return;

		DWORD mode = 0;
		if (!GetConsoleMode(h, &mode))
			return;

		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(h, mode);
	};

	enableFor(STD_OUTPUT_HANDLE);
	enableFor(STD_ERROR_HANDLE);
#endif
}

static void SigIntHandler(int signal)
{
	exit(SIGINT);
}

static bool CheckFilesExisting()
{
	bool anyUnrealFiles = false;
	for (const std::wstring& file : ConsoleArguments::FilesToCompile)
	{
		if (!fs::exists(file))
		{
			anyUnrealFiles = true;
			std::wcout << L"'" << file << L"' doesn't exists";
		}
	}

	return !anyUnrealFiles;
}

static void LoadLibrariesFromDirectoryPath(CompilationContext* compiler, fs::path path)
{
	for (const fs::directory_entry& entry : fs::directory_iterator(path))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filename = entry.path().filename();
#if defined(_WIN32)
		if (!filename.string().ends_with(".dll"))
#else
		if (!filename.string().ends_with(".so"))
#endif
			continue;

		try
		{
			compiler->AddLib(entry.path().wstring());
		}
		catch (std::runtime_error& err)
		{
			std::cout << err.what();
		}
	}
}

static fs::path GetCurrentDirectoryPath()
{
#if defined(_WIN32)
	// Windows implementation
	WCHAR pathBuffer[MAX_PATH];
	DWORD size = GetModuleFileNameW(NULL, pathBuffer, static_cast<DWORD>(MAX_PATH));
	return fs::path(pathBuffer).parent_path();

#elif defined(__linux__)
	// Linux Implementation
	char pathBuffer[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", pathBuffer, sizeof(pathBuffer) - 1);
	if (len == -1)
		return "";
	pathBuffer[len] = '\0';
	return std::filesystem::path(pathBuffer).parent_path();

#else
	return ""; // Unsupported OS
#endif
}

static fs::path GetWorkingDirectoryPath()
{
	return fs::current_path();
}

static std::wstring ReadSourceLine(const std::wstring& filePath, int targetLine)
{
	std::filesystem::path sourcePath(filePath);
	std::wifstream in(sourcePath);
	in.imbue(std::locale::classic());
	if (!in.is_open())
		return L"";

	std::wstring line;
	int currentLine = 1;
	while (currentLine < targetLine && std::getline(in, line))
		++currentLine;

	if (!std::getline(in, line))
		return L"";

	if (!line.empty() && line.back() == L'\r')
		line.pop_back();

	return line;
}

static const wchar_t* SeverityText(shard::DiagnosticSeverity severity)
{
	switch (severity)
	{
		case shard::DiagnosticSeverity::Warning: return L"warning";
		case shard::DiagnosticSeverity::Info:    return L"info";
		default:                                 return L"error";
	}
}

static const wchar_t* SeverityColor(shard::DiagnosticSeverity severity)
{
	switch (severity)
	{
		case shard::DiagnosticSeverity::Warning: return CYellow;
		case shard::DiagnosticSeverity::Info:    return CBlue;
		default:                                 return CRed;
	}
}

static void PrintDiagnostics(DiagnosticsContext& diagnostics)
{
	for (const shard::Diagnostic& diag : diagnostics.Diagnostics)
	{
		const shard::TextLocation& loc = diag.Location;
		const wchar_t* severityColor = SeverityColor(diag.Severity);

		std::wcout << severityColor << SeverityText(diag.Severity) << CReset
		           << L": " << diag.Description << std::endl;

		std::wcout << CGray << L" --> " << CReset
		           << CCyan << loc.FileName << CReset
		           << CGray << L":" << loc.Line << L":" << loc.Offset << CReset << std::endl;

		std::wcout << CGray << L"    |" << CReset << std::endl;

		std::wstring sourceLine = ReadSourceLine(loc.FileName, loc.Line);
		if (!sourceLine.empty())
		{
			constexpr int MaxVisibleLineLength = 160;
			constexpr int CaretWindowRadius = 60;

			int start = (std::max)(0, loc.Offset - 1);
			int length = (std::max)(1, loc.Length);

			if (start > static_cast<int>(sourceLine.length()))
				start = static_cast<int>(sourceLine.length());

			if (start + length > static_cast<int>(sourceLine.length()))
				length = static_cast<int>(sourceLine.length()) - start;

			// If the source line is extremely long, show only a window around the
			// diagnostic so the output stays readable.
			std::wstring displayedLine = sourceLine;
			int lineOffset = 0;
			bool trimmedPrefix = false;
			bool trimmedSuffix = false;

			if (static_cast<int>(sourceLine.length()) > MaxVisibleLineLength)
			{
				int windowStart = (std::max)(0, start - CaretWindowRadius);
				int windowEnd = (std::min)(static_cast<int>(sourceLine.length()), start + length + CaretWindowRadius);

				if (windowStart > 0)
				{
					trimmedPrefix = true;
					windowStart = (std::max)(0, windowStart - 4);
				}

				if (windowEnd < static_cast<int>(sourceLine.length()))
				{
					trimmedSuffix = true;
					windowEnd = (std::min)(static_cast<int>(sourceLine.length()), windowEnd + 4);
				}

				displayedLine = sourceLine.substr(windowStart, windowEnd - windowStart);
				lineOffset = windowStart;
			}

			std::wcout << CGray << std::setw(3) << loc.Line << L" | " << CReset;
			if (trimmedPrefix)
				std::wcout << L"...";

			std::wcout << displayedLine;
			if (trimmedSuffix)
				std::wcout << L"...";

			std::wcout << std::endl;

			int caretStart = start - lineOffset;
			if (trimmedPrefix)
				caretStart += 3;

			std::wcout << CGray << L"    | " << CReset
			           << std::wstring(caretStart, L' ')
			           << severityColor << std::wstring(length, L'^') << CReset << std::endl;
		}
	}
}

static void PrintPrettyUnhandledException(
	ObjectInstance* exception,
	const std::wstring& message,
	const std::wstring& stackTrace,
	SymbolTable* table)
{
	std::wstring exceptionType = (exception != nullptr && exception->getInfo() != nullptr)
		? exception->getInfo()->Name
		: L"UnknownException";

	std::wcerr << std::endl;
	std::wcerr << CRed << CBold << L"Unhandled exception." << CReset;
	std::wcerr << L" " << CCyan << exceptionType << CReset;
	std::wcerr << CRed << L":" << CReset;
	std::wcerr << L" " << CRed << message << CReset;
	std::wcerr << std::endl;

	std::wcerr << std::endl;
	std::wcerr << CWhite << L"Stack trace:" << CReset << std::endl;

	std::unordered_map<std::wstring, MethodSymbol*> methodMap;
	if (table != nullptr)
	{
		for (MethodSymbol* method : table->GetMethodSymbols())
		{
			if (method != nullptr)
				methodMap[method->FullName] = method;
		}
	}

	std::wistringstream traceReader(stackTrace);
	std::wstring frameName;
	bool anyFrame = false;

	while (std::getline(traceReader, frameName))
	{
		if (frameName.empty())
			continue;

		anyFrame = true;

		std::wcerr << std::endl;
		std::wcerr << CGray << L"   at " << CReset
		           << CCyan << frameName << CReset;

		MethodSymbol* method = nullptr;
		auto it = methodMap.find(frameName);
		if (it != methodMap.end())
			method = it->second;

		if (method != nullptr && table != nullptr)
		{
			MethodDeclarationSyntax* methodNode = static_cast<MethodDeclarationSyntax*>(
				table->LookupNode(method).value_or(nullptr));

			if (methodNode != nullptr)
			{
				const shard::TextLocation& loc = methodNode->IdentifierToken.Location;
				if (!loc.FileName.empty())
				{
					std::wcerr << CGray << L" in " << CReset
					           << CBlue << loc.FileName << CReset
					           << CGray << L":line " << CReset
					           << CWhite << loc.Line << CReset;

					/*
					std::wstring sourceLine = ReadSourceLine(loc.FileName, loc.Line);
					if (!sourceLine.empty())
					{
						std::wcerr << std::endl;
						std::wcerr << CGray << L"      " << std::setw(4) << loc.Line << L" | " << CReset
						           << sourceLine << std::endl;

						int start = (std::max)(0, loc.Offset - 1);
						int length = (std::max)(1, loc.Length);

						if (start > static_cast<int>(sourceLine.length()))
							start = static_cast<int>(sourceLine.length());

						if (start + length > static_cast<int>(sourceLine.length()))
							length = static_cast<int>(sourceLine.length()) - start;

						std::wcerr << CGray << std::wstring(11, L' ') << L"| " << CReset
						           << std::wstring(start, L' ')
						           << CRed << std::wstring(length, L'^') << CReset;
					}
					*/
				}
			}
		}
	}

	if (!anyFrame)
	{
		std::wcerr << CGray << L"   <no stack trace available>" << CReset << std::endl;
	}

	std::wcerr << std::endl;
}

static void PrintCriticalError(const char* message)
{
	std::wcerr << std::endl;
	std::wcerr << CRed << CBold << L"Critical error:" << CReset
	           << L" " << CWhite << message << CReset << std::endl;
	std::wcerr << std::endl;
}

static CompilationUnitSyntax* GetCompilationUnit(SyntaxNode* node)
{
	while (node != nullptr && node->Kind != SyntaxKind::CompilationUnit)
		node = node->Parent;

	return static_cast<CompilationUnitSyntax*>(node);
}

int wmain(int argc, wchar_t* argv[])
{
	CompilationContext compiler;

	try
	{
		setlocale(LC_ALL, "");
		EnableTerminalColors();
		signal(SIGINT, SigIntHandler);
		ShardUtilities::ParseArguments(argc, argv);

		if (ConsoleArguments::ShowHelp)
		{
			std::wcout << std::endl << L"ShardLang interpreter v0.2.0" << std::endl << std::endl;

			std::wcout << L"\t> '-h', '--help'        \t Show this help screen." << std::endl;
			std::wcout << L"\t> '-i', '--interactive' \t Run REPL console" << std::endl;
			std::wcout << L"\t> '-d', '--decompiled'  \t Instead of running program, decompile it entry point and print its bytecode" << std::endl;
			std::wcout << L"\t> '--no-std'	          \t Prevents loading standard library from STD directory" << std::endl;
			return 0;
		}

		if (!CheckFilesExisting())
			return 1;

		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		compiler.SetEntryPoint = ConsoleArguments::RunProgram || ConsoleArguments::ShowDecompile;

		if (!ConsoleArguments::ExcludeStd)
		{
			fs::path currentDirectory = GetCurrentDirectoryPath() / L"system";
			LoadLibrariesFromDirectoryPath(&compiler, currentDirectory);
		}

		for (const std::wstring& file : ConsoleArguments::LibsToLoad)
		{
			compiler.AddLib(file);
		}

		fs::path workingDirectory = GetWorkingDirectoryPath();
		if (!fs::exists(workingDirectory))
		{
			std::wcout << L"Could not resolve current working directory!" << std::endl;
			return 1;
		}

		for (const std::wstring& file : ConsoleArguments::FilesToCompile)
		{
			FileReader reader(file);
			LexicalAnalyzer lexer(reader);
			compiler.EnrichTree(lexer, CompilationUnitOrigin::SourceFile);
		}

		if (ConsoleArguments::UseInteractive)
		{
			compiler.SetPopExpressionStatement(false);
		}

		auto domain = compiler.Compile();
		if (diagnostics.AnyError)
			throw diagnostics_exception("Compilation ended with errors.");

		if (ConsoleArguments::UseInteractive)
		{
			InteractiveConsole repl(&compiler, domain.get());
			repl.Run();
			return 0;
		}

		if (ConsoleArguments::ShowDecompile)
		{
			SymbolTable* table = compiler.GetSemanticModel().Table.get();
			ProgramDisassembler disassembler;

			const std::vector<MethodSymbol*>& methods = table->GetMethodSymbols();
			for (const auto& method : methods)
			{
				if (method->HandleType != MethodHandleType::Body)
					continue;

				MethodDeclarationSyntax* methodNode = static_cast<MethodDeclarationSyntax*>(table->LookupNode(method).value_or(nullptr));
				if (methodNode == nullptr)
				{
					std::cout << "Failed to resolve methods node" << std::endl;
					continue;
				}

				CompilationUnitSyntax* unit = GetCompilationUnit(methodNode);
				if (unit == nullptr)
				{
					std::wcout << L"Failed to resolve " << methodNode->IdentifierToken.Word << L" methods unit" << std::endl;
					continue;
				}

				if (unit->Origin != CompilationUnitOrigin::SourceFile)
					continue;

				disassembler.Disassemble(std::wcout, method);
			}
		}

		if (ConsoleArguments::RunProgram)
		{
			VirtualMachine& virtualMachine = domain->GetVirtualMachine();
			SymbolTable* symbolTable = compiler.GetSemanticModel().Table.get();
			virtualMachine.Run();
			ConsoleHelper::Write(L"\n");

			ObjectInstance* unhandledException = virtualMachine.GetUnhandledException();
			if (unhandledException != nullptr)
			{
				PrintPrettyUnhandledException(
					unhandledException,
					virtualMachine.GetUnhandledExceptionMessage(),
					virtualMachine.GetUnhandledExceptionStackTrace(),
					symbolTable);

				return 1;
			}
		}
	}
	catch (const diagnostics_exception& err)
	{
		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		if (diagnostics.AnyError)
		{
			PrintDiagnostics(diagnostics);
			return 1;
		}
	}
	catch (const std::runtime_error& err)
	{
		PrintCriticalError(err.what());
	}
	catch (const std::exception& err)
	{
		PrintCriticalError(err.what());
		return 3;
	}
	catch (...)
	{
		PrintCriticalError("unknown exception");
		return 3;
	}

	return 0;
}

#if !defined(_WIN32)
int main(int argc, char* argv[])
{
	std::setlocale(LC_ALL, "");

	std::vector<std::wstring> wideArgs;
	wideArgs.reserve(argc);
	for (int i = 0; i < argc; ++i)
		wideArgs.push_back(std::filesystem::path(argv[i]).wstring());

	std::vector<wchar_t*> wideArgv;
	wideArgv.reserve(argc + 1);
	for (auto& arg : wideArgs)
		wideArgv.push_back(const_cast<wchar_t*>(arg.data()));

	wideArgv.push_back(nullptr);
	return wmain(argc, wideArgv.data());
}
#endif
