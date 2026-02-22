#pragma once
#include <shard/parsing/lexical/LexicalBuffer.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/SyntaxTree.hpp>

namespace shard
{
	class InteractiveConsole
	{
		SyntaxTree& ParentSyntaxTree;
		SemanticModel& ParentSemanticModel;
		DiagnosticsContext& Diagnostics;

		SourceParser Parser;
		SemanticAnalyzer Semanter;
		LayoutGenerator Layouter;

		CompilationUnitSyntax* InteractiveUnit;
		ClassDeclarationSyntax* InteractiveClass;
		MethodDeclarationSyntax* InteractiveMethod;
		MethodSymbol* InteractiveEntryPoint;

		ProgramVirtualImage Program;
		VirtualMachine Runtimer;
		size_t Breakpoint = 0;

		void EvaluateUsing(LexicalBuffer& buffer);
		MemberDeclarationSyntax* ReadMember(LexicalBuffer& sequenceReader);
		StatementSyntax* ReadStatement(LexicalBuffer& sequenceReader);

	public:
		InteractiveConsole(SyntaxTree& syntaxTree, SemanticModel& semanticModel, DiagnosticsContext& diagnostics);

		void Run();
	};
}
