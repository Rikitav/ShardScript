#pragma once
#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/TypeDeclarations.h>
#include <memory>

using namespace shard::syntax::structures;
using namespace shard::syntax::nodes;

namespace shard::parsing
{
	class SyntaxTreeParser
	{
		DiagnosticsContext& Diagnostics;

	public:
		SyntaxTreeParser(DiagnosticsContext& diagnostics) : Diagnostics(diagnostics)
		{

		}

		void EnsureSyntaxTree(shared_ptr<SyntaxTree> tree);

	//private:
		void EnsureCompilationUnit(shared_ptr<CompilationUnitSyntax> syntax);
		void EnsureMemberDeclaration(shared_ptr<MemberDeclarationSyntax> syntax);
		void EnsureClassDeclaration(shared_ptr<ClassDeclarationSyntax> syntax);
		void EnsureMethodDeclaration(shared_ptr<MethodDeclarationSyntax> syntax);
		void EnsureStatement(shared_ptr<StatementSyntax> syntax);
		void EnsureExpression(shared_ptr<ExpressionSyntax> syntax);
	};
}
