#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/ShardScriptLIB.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/lexical/reading/SourceTextProvider.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <filesystem>

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
		void ProvideSource(shard::SourceTextProvider* source);
		void ProvideSource(shard::SourceProvider* source);

		void EnrichTree(SourceProvider& sourceProvider, CompilationUnitOrigin origin);
		void AnalyzeTree();

		ApplicationDomain* Compile();
	};
}