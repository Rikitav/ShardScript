#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <ios>
#include <vector>

#include <ShardScript.hpp>
#include <InteractiveConsole.hpp>
#include <utilities/Console.hpp>
#include <utilities/Diagnostics.hpp>
#include <utilities/Exceptions.hpp>

#include "utilities/flag.h"
#include "utilities/whereami.h"
#include "utilities/glob.h"

using namespace shard;
namespace fs = std::filesystem;

namespace
{
	static std::optional<std::vector<std::string>> GetFilesToCompile(Flag_Context* flagCtx)
	{
		std::vector<std::string> filesToCompile;

		int restCount = flag_c_rest_argc(flagCtx);
		char** restArgs = flag_c_rest_argv(flagCtx);

		for (int i = 0; i < restCount; ++i)
		{
			bool globbed = false;
			glob::glob glob(restArgs[i]);
			glob.use_full_paths(true);

			while (glob)
			{
				globbed = true;
				filesToCompile.push_back(glob.current_match());
				glob.next();
			}

			if (!globbed)
			{
				std::cout << "Error: Source file not found '" << restArgs[i] << "'.\n";
				return std::nullopt;
			}
		}

		return filesToCompile;
	}

	static void LoadLibrariesFromDirectoryPath(CompilationContext* compiler, const fs::path& path)
	{
		std::vector<fs::path> libraryPaths;
		for (const auto& entry : fs::directory_iterator(path))
		{
			if (!entry.is_regular_file())
				continue;

			const auto extension = entry.path().extension();
#if defined(_WIN32)
			if (extension != ".dll")
#else
			if (extension != ".so")
#endif
				continue;

			libraryPaths.push_back(entry.path());
		}

		compiler->AddLibraries(libraryPaths);
	}
}

int main(int argc, char* argv[])
{
	CompilationContext compiler;
	Flag_Context* flagCtx = static_cast<Flag_Context*>(flag_c_new(nullptr));

	bool Arg_UseInteractive = false;
	bool Arg_ShowHelp = false;
	bool Arg_ExcludeStd = false;
	bool Arg_ShowDecompile = false;
	bool Arg_LintOnly = false;
	Flag_List_Mut Args_Libraries{};

	try
	{
		setlocale(LC_ALL, "");
		console::EnableColors();

		flag_c_bool_var(flagCtx, &Arg_ShowHelp, "help", false, "Show this help screen");
		flag_c_set_short_name_by_name(flagCtx, "help", "h");

		flag_c_bool_var(flagCtx, &Arg_UseInteractive, "interactive", false, "Run REPL console");
		flag_c_set_short_name_by_name(flagCtx, "interactive", "i");

		flag_c_bool_var(flagCtx, &Arg_UseInteractive, "repl", false, "Run REPL console");
		flag_c_set_short_name_by_name(flagCtx, "repl", "r");

		flag_c_bool_var(flagCtx, &Arg_ShowDecompile, "decompiled", false, "Instead of running program, decompile its entry point and print bytecode");
		flag_c_set_short_name_by_name(flagCtx, "decompiled", "d");

		flag_c_bool_var(flagCtx, &Arg_LintOnly, "lint", false, "Check source files for errors without running them");

		flag_c_bool_var(flagCtx, &Arg_ExcludeStd, "no-std", false, "Prevent loading standard library from STD directory");
		flag_c_bool_var(flagCtx, &Arg_ExcludeStd, "exclude-std", false, "Prevent loading standard library from STD directory");

		flag_c_list_mut_var(flagCtx, &Args_Libraries, "library", "Load a native library (can be repeated)");
		flag_c_set_short_name_by_name(flagCtx, "library", "l");

		if (!flag_c_parse(flagCtx, argc, argv))
		{
			flag_c_print_error(flagCtx, stderr);
			flag_c_free(flagCtx);
			return 1;
		}

		if (Arg_ShowHelp)
		{
			std::string filename = whereami::executable_name();

			std::cout << std::endl;
			std::cout << "ShardLang interpreter v" << SHARDSCRIPT_VERSION << std::endl;
			std::cout << std::endl;
			std::cout << "Usage: ./" << filename << " [OPTIONS] [--] [ARGS]\n" << std::endl;
			std::cout << "OPTIONS:" << std::endl;

			flag_c_print_options(flagCtx, stdout);
			flag_c_free(flagCtx);
			return 0;
		}

		std::optional<std::vector<std::string>> filesToCompileOpt = GetFilesToCompile(flagCtx);
		flag_c_free(flagCtx);
		flagCtx = nullptr;

		if (!filesToCompileOpt.has_value())
			return 1;

		std::vector<std::string> FilesToCompile = filesToCompileOpt.value();

		const bool hasFiles = !FilesToCompile.empty();
		const bool RunProgram = hasFiles && !Arg_LintOnly && !Arg_ShowDecompile && !Arg_UseInteractive && !Arg_ShowHelp;
		const bool RunInteractive = Arg_UseInteractive || (!RunProgram && !Arg_ShowHelp && !Arg_ShowDecompile && !Arg_LintOnly && !hasFiles);

		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		compiler.SetEntryPoint = RunProgram || Arg_ShowDecompile;

		fs::path workingDirectory = fs::current_path();
		if (!fs::exists(workingDirectory))
		{
			std::wcout << L"Could not resolve current working directory!" << std::endl;
			return 1;
		}

		if (!Arg_ExcludeStd)
		{
			fs::path currentDirectory = fs::path(whereami::executable_dir()) / "system";
			LoadLibrariesFromDirectoryPath(&compiler, currentDirectory);
		}

		for (size_t i = 0; i < Args_Libraries.count; ++i)
		{
			compiler.AddLib(fs::path(Args_Libraries.items[i]));
		}

		for (const std::string& file : FilesToCompile)
		{
			FileReader reader(file);
			LexicalAnalyzer lexer(reader);
			compiler.EnrichTree(lexer, CompilationUnitOrigin::SourceFile);
		}

		if (Arg_LintOnly)
		{
			compiler.AnalyzeTree();
			if (diagnostics.AnyError)
				throw diagnostics_exception("Lint ended with errors.");

			return 0;
		}

		std::unique_ptr<ApplicationDomain> domain = compiler.Compile();
		if (diagnostics.AnyError)
			throw diagnostics_exception("Compilation ended with errors.");

		if (RunInteractive)
		{
			compiler.SetPopExpressionStatement(false);
			InteractiveConsole repl(&compiler, domain.get());
			repl.Run();
			return 0;
		}

		if (Arg_ShowDecompile)
		{
			ProgramDisassembler disassembler;
			disassembler.Disassemble(std::wcout, compiler);
			return 0;
		}

		if (RunProgram)
		{
			VirtualMachine& virtualMachine = domain->GetVirtualMachine();
			SymbolTable* symbolTable = compiler.GetSemanticModel().Table.get();
			virtualMachine.Run();
			ConsoleHelper::Write(L"\n");

			ObjectInstance* unhandledException = virtualMachine.GetUnhandledException();
			if (unhandledException != nullptr)
			{
				exceptions::PrintUnhandled(
					std::wcerr,
					unhandledException,
					virtualMachine.GetUnhandledExceptionMessage(),
					virtualMachine.GetUnhandledExceptionStackTrace(),
					symbolTable);

				return 1;
			}

			return 0;
		}

		std::cout << "Nothing to do." << std::endl;
		return 0;
	}
	catch (const diagnostics_exception& err)
	{
		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		if (diagnostics.AnyError)
		{
			diagnostics::Print(std::wcout, diagnostics);
			return 1;
		}
	}
	catch (const std::runtime_error& err)
	{
		exceptions::PrintCritical(std::wcerr, err.what());
	}
	catch (const std::exception& err)
	{
		exceptions::PrintCritical(std::wcerr, err.what());
		return 3;
	}
	catch (...)
	{
		exceptions::PrintCritical(std::wcerr, "unknown exception");
		return 3;
	}

	if (flagCtx != nullptr)
		flag_c_free(flagCtx);

	return 0;
}
