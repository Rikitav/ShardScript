#pragma once
#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>

using namespace shard::syntax::structures;

namespace shard::parsing
{
	class SyntaxTreeParser
	{
		DiagnosticsContext& Diagnostics;

	public:
		SyntaxTreeParser(DiagnosticsContext& diagnostics) : Diagnostics(diagnostics)
		{

		}

		void EnsureNormalizedSyntax(shared_ptr<SyntaxTree> tree);

	//private:
		void EnsureCompilationUnit(shared_ptr<CompilationUnitSyntax> syntax);
		void EnsureMemberDeclaration(shared_ptr<MemberDeclarationSyntax> syntax);
		void EnsureClassDeclaration(shared_ptr<ClassDeclarationSyntax> syntax);
		void EnsureMethodDeclaration(shared_ptr<MethodDeclarationSyntax> syntax);
		void EnsureStatement(shared_ptr<StatementSyntax> syntax);
		void EnsureExpression(shared_ptr<ExpressionSyntax> syntax);
	};
}
