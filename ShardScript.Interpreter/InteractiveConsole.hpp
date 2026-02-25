#pragma once
#include <ShardScript.hpp>

namespace shard
{
	class InteractiveConsole
	{
		SyntaxTree& ParentSyntaxTree;
		SemanticModel& ParentSemanticModel;
		DiagnosticsContext& Diagnostics;

		SourceParser& Parser;
		SemanticAnalyzer& Semanter;
		LayoutGenerator& Layouter;

		CompilationUnitSyntax* InteractiveUnit;
		ClassDeclarationSyntax* InteractiveClass;
		MethodDeclarationSyntax* InteractiveMethod;
		MethodSymbol* InteractiveEntryPoint;

		shard::CompilationContext* compilationContext;
		shard::ApplicationDomain* applicationDomain;
		
		ProgramVirtualImage& Program;
		VirtualMachine& Runtimer;
		size_t Breakpoint = 0;

		void EvaluateUsing(LexicalBuffer& buffer);
		MemberDeclarationSyntax* ReadMember(LexicalBuffer& sequenceReader);
		StatementSyntax* ReadStatement(LexicalBuffer& sequenceReader);

	public:
		InteractiveConsole(shard::CompilationContext* context, shard::ApplicationDomain* domain);

		void Run();
	};
}
