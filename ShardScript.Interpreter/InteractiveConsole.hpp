#pragma once
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/lexical/LexicalBuffer.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <shard/CompilationContext.hpp>

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

		CompilationContext* compilationContext;
		ApplicationDomain* applicationDomain;
		
		ProgramVirtualImage& Program;
		VirtualMachine& Runtimer;
		size_t Breakpoint = 0;

		void EvaluateUsing(LexicalBuffer& buffer);
		MemberDeclarationSyntax* ReadMember(LexicalBuffer& sequenceReader);
		StatementSyntax* ReadStatement(LexicalBuffer& sequenceReader);

	public:
		InteractiveConsole(CompilationContext* context, ApplicationDomain* domain);

		void Run();
	};
}
