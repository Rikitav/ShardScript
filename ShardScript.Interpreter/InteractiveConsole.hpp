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

		CompilationUnitSyntax* InteractiveUnit = nullptr;
		ClassDeclarationSyntax* InteractiveClass = nullptr;
		MethodDeclarationSyntax* InteractiveMethod = nullptr;
		MethodSymbol* InteractiveEntryPoint = nullptr;

		CompilationContext* compilationContext = nullptr;
		ApplicationDomain* applicationDomain = nullptr;
		
		ProgramVirtualImage& Program;
		VirtualMachine& Runtimer;
		std::size_t Breakpoint = 0;

		void EvaluateUsing(LexicalBuffer& buffer);
		MemberDeclarationSyntax* ReadMember(LexicalBuffer& sequenceReader);
		StatementSyntax* ReadStatement(LexicalBuffer& sequenceReader);

	public:
		InteractiveConsole(CompilationContext* context, ApplicationDomain* domain);

		void Run();
	};
}
