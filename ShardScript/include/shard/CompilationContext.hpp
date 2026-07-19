#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/ShardScriptLIB.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/SourceTextProvider.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>
#include <shard/compilation/LayoutGenerator.hpp>

#include <filesystem>
#include <memory>

namespace shard
{
	class SHARD_API CompilationContext
	{
		std::vector<LibraryHandle> LibHandles;
		std::vector<CompilationUnitSyntax*> PendingSources;

		SyntaxTree Tree;
		SemanticModel Model;
		DiagnosticsContext Diagnostics;

		SourceParser Parser;
		SemanticAnalyzer Semanter;
		LayoutGenerator Layouter;

		bool PopExpressionStatement = true;
		bool ReAnalyze = true;

	public:
		bool SetEntryPoint = true;

		CompilationContext();
		~CompilationContext();

		CompilationContext(const CompilationContext&) = delete;
		CompilationContext& operator=(const CompilationContext&) = delete;

		void SetPopExpressionStatement(bool pop)
		{
			PopExpressionStatement = pop;
		}

		SyntaxTree& GetSyntaxTree();
		SemanticModel& GetSemanticModel();
		DiagnosticsContext& GetDiagnosticsContext();

		SourceParser& GetParser();
		SemanticAnalyzer& GetSemanticAnalyzer();
		LayoutGenerator& GetLayoutGenerator();

		void AddLib(const std::filesystem::path& path);
		void AddLib(const LibraryHandle& handle);
		void AddLibraries(const std::vector<std::filesystem::path>& paths);

		void ProvideSource(shard::SourceTextProvider* source);

		void EnrichTree(SourceProvider& sourceProvider, CompilationUnitOrigin origin);
		void AnalyzeTree();
		void MarkForReAnalyze() { ReAnalyze = true; }

		std::unique_ptr<ApplicationDomain> Compile();
	};
}