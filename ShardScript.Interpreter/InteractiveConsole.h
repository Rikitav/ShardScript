#pragma once
#include <shard/parsing/lexical/LexicalBuffer.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/SyntaxTree.h>

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
