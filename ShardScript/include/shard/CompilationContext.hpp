#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/FrameworkModule.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <filesystem>

namespace shard
{
	class SHARD_API CompilationContext
	{
		std::vector<LibraryHandle> LibHandles;
		std::vector<FrameworkModule*> LibModules;

		SyntaxTree Tree;
		SemanticModel Model;
		DiagnosticsContext Diagnostics;

		SourceParser Parser;
		SemanticAnalyzer Semanter;
		LayoutGenerator Layouter;

		bool ReAnalyze = true;

	public:
		bool SetEntryPoint = true;

		CompilationContext();
		~CompilationContext();

		SyntaxTree& GetSyntaxTree();
		SemanticModel& GetSemanticModel();
		DiagnosticsContext& GetDiagnosticsContext();

		SourceParser& GetParser();
		SemanticAnalyzer& GetSemanticAnalyzer();
		LayoutGenerator& GetLayoutGenerator();

		void AddLib(const std::filesystem::path& path);
		void AddLib(const LibraryHandle& handle);

		void EnrichTree(SourceProvider& sourceProvider);
		void AnalyzeTree();

		ApplicationDomain* Compile();
	};
}