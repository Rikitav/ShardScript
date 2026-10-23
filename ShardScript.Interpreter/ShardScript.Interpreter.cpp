#include <clocale>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/Definitions.hpp>

#include <shard/compilation/ProgramDisassembler.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/parsing/Diagnostics.hpp>

#include <shard/runtime/VirtualMachine.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Strings.hpp>

#include "StandardLibrary.hpp"

#include "utilities/flag.h"
#include "utilities/glob.h"
#include "utilities/whereami.h"

#if defined(_WIN32)
#include <windows.h>
#endif

using namespace shard;
using namespace shard::interpreter;

namespace
{
	void enable_console_colors()
	{
#if defined(_WIN32)
		const HANDLE handles[] = { GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE) };
		for (const HANDLE handle : handles)
		{
			if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
				continue;

			DWORD mode = 0;
			if (!GetConsoleMode(handle, &mode))
				continue;

			SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
#endif
	}

	std::optional<gmt::vector<gmt::string>> get_files_to_compile(Flag_Context* flagCtx)
	{
		gmt::vector<gmt::string> filesToCompile;

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

	void load_libraries_from_directory(CompilationContext& context, const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
			return;

		gmt::vector<std::filesystem::path> libraryPaths;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path))
		{
			if (!entry.is_regular_file())
				continue;

			const std::filesystem::path extension = entry.path().extension();
#if defined(_WIN32)
			if (extension != ".dll")
#else
			if (extension != ".so")
#endif
				continue;

			libraryPaths.push_back(entry.path());
		}

		context.add_libraries(libraryPaths);
	}

	// Prints pending diagnostics; returns false when errors were reported.
	// Warnings are flushed and forgotten, so later phases only print fresh ones.
	bool flush_diagnostics(DiagnosticsContext& diagnostics)
	{
		if (!diagnostics.has_errors() && !diagnostics.has_warns())
			return true;

		diagnostics.write_diagnostics_pretty(std::wcout);
		if (diagnostics.has_errors())
			return false;

		diagnostics.reset();
		return true;
	}

	void print_help(Flag_Context* flagCtx)
	{
		gmt::string filename = whereami::executable_name();

		std::cout << std::endl;
		std::cout << "ShardScript interpreter v" << SHARDSCRIPT_VERSION << std::endl;
		std::cout << std::endl;
		std::cout << "Usage: " << filename << " [OPTIONS] <files...> [--] [ARGS]\n" << std::endl;
		std::cout << "OPTIONS:" << std::endl;

		flag_c_print_options(flagCtx, stdout);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	enable_console_colors();

	char** argvInterpreter = argv;
	int argcInterpreter = argc;
	int argcScripts = 0;

	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--") == 0)
		{
			argcScripts = argc - i - 1;
			argcInterpreter = argc - argcScripts - 1;
			break;
		}
	}

	Flag_Context* flagCtx = static_cast<Flag_Context*>(flag_c_new(nullptr));

	bool argShowHelp = false;
	bool argShowDecompile = false;
	bool argLintOnly = false;
	bool argExcludeStd = false;
	Flag_List_Mut argsLibraries{};

	flag_c_bool_var(flagCtx, &argShowHelp, "help", false, "Show this help screen");
	flag_c_set_short_name_by_name(flagCtx, "help", "h");

	flag_c_bool_var(flagCtx, &argShowDecompile, "decompiled", false, "Instead of running program, print its bytecode");
	flag_c_set_short_name_by_name(flagCtx, "decompiled", "d");

	flag_c_bool_var(flagCtx, &argLintOnly, "lint", false, "Check source files for errors without running them");

	flag_c_bool_var(flagCtx, &argExcludeStd, "no-std", false, "Prevent loading standard libraries from the system directory");
	flag_c_bool_var(flagCtx, &argExcludeStd, "exclude-std", false, "Prevent loading standard libraries from the system directory");

	flag_c_list_mut_var(flagCtx, &argsLibraries, "library", "Load a native ShardScript library (can be repeated)");
	flag_c_set_short_name_by_name(flagCtx, "library", "l");

	if (!flag_c_parse(flagCtx, argcInterpreter, argvInterpreter))
	{
		flag_c_print_error(flagCtx, stderr);
		flag_c_free(flagCtx);
		return 1;
	}

	if (argShowHelp)
	{
		print_help(flagCtx);
		flag_c_free(flagCtx);
		return 0;
	}

	std::optional<gmt::vector<gmt::string>> filesToCompileOpt = get_files_to_compile(flagCtx);
	flag_c_free(flagCtx);

	if (!filesToCompileOpt.has_value())
		return 1;

	const gmt::vector<gmt::string> filesToCompile = std::move(filesToCompileOpt.value());
	const bool hasFiles = !filesToCompile.empty();

	try
	{
		if (!hasFiles)
		{
			std::cout << "Nothing to do. Run with --help to see usage." << std::endl;
			return 0;
		}

		CompilationContext context;
		DiagnosticsContext& diagnostics = context.get_diagnostics();

		if (!argExcludeStd)
		{
			std::filesystem::path systemDirectory = std::filesystem::path(whereami::executable_dir()) / "system";
			load_libraries_from_directory(context, systemDirectory);
		}

		for (std::size_t i = 0; i < argsLibraries.count; ++i)
			context.add_lib(std::filesystem::path(argsLibraries.items[i]));

		if (!flush_diagnostics(diagnostics))
			return 1;

		for (const gmt::string& file : filesToCompile)
			context.parse_file(gmt::utf8_to_wide(file));

		if (!flush_diagnostics(diagnostics))
			return 1;

		register_standard_library(context.get_semantic_model());

		context.analyze_tree();
		if (!flush_diagnostics(diagnostics))
			return 1;

		if (argLintOnly)
			return 0;

		context.emit_program();
		if (!flush_diagnostics(diagnostics))
			return 1;

		if (argcScripts != 0 && !argShowDecompile)
			std::wcout << L"warning: script arguments are not supported yet and will be ignored" << std::endl;

		gmt::unique_ptr<ApplicationDomain> domain = context.compile();
		if (argShowDecompile)
		{
			ProgramDisassembler disassembler(*domain);
			disassembler.disassemble(std::wcout);
			return 0;
		}

		VirtualMachine& machine = domain->get_virtual_machine();
		machine.run();

		if (machine.get_has_unhandled_exception())
		{
			std::wcerr << L"Unhandled exception: " << machine.get_unhandled_message() << std::endl;
			return 1;
		}

		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "critical error: " << err.what() << std::endl;
		return 3;
	}
	catch (...)
	{
		std::cerr << "critical error: unknown exception" << std::endl;
		return 3;
	}
}
