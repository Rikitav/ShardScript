#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/MethodBodySyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <memory>

using namespace std;
using namespace shard::parsing::structures;

namespace shard::syntax::nodes
{
	class MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnType;
		shared_ptr<MethodBodySyntax> Body;
		shared_ptr<ParametersListSyntax> Params;

		MethodDeclarationSyntax() : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration), Body(nullptr), Params(nullptr)
		{

		}

		MethodDeclarationSyntax(MemberDeclarationInfo& info) : MemberDeclarationSyntax(SyntaxKind::ClassDeclaration), Body(nullptr), Params(nullptr)
		{
			Modifiers = info.Modifiers;
			Identifier = info.Identifier;
			ReturnType = info.ReturnType;
		}

		void ReportMissing(DiagnosticsContext& diagnostics)
		{
			if (ReturnType.IsMissing)
				diagnostics.ReportError(DeclareKeyword, "Missing return type of method");

			//if (Body != nullptr)
				//Body->ReportMissing(diagnostics);
			
			//if (Params != nullptr)
				//Params.ReportMissing(diagnostics);
		}
	};
}
