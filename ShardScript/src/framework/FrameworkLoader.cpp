#include <shard/framework/FrameworkLoader.h>
#include <shard/framework/FrameworkModule.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/reading/StringStreamReader.h>

#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

using namespace shard::syntax::nodes;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

std::vector<FrameworkModule*> FrameworkLoader::Modules;

void FrameworkLoader::AddModule(FrameworkModule* pModule)
{
	Modules.push_back(pModule);
}

void FrameworkLoader::Load(SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	for (FrameworkModule* module : Modules)
	{
		LoadSingleModule(module, semanticModel, diagnostics);
		delete module;
	}

	Modules.clear();
}

static void BindMemberDeclaration(MemberDeclarationSyntax* member, FrameworkModule* module, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	switch (member->Kind)
	{
		case SyntaxKind::NamespaceDeclaration:
		{
			NamespaceDeclarationSyntax* namespaceDecl = static_cast<NamespaceDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : namespaceDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::ClassDeclaration:
		{
			ClassDeclarationSyntax* classDecl = static_cast<ClassDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : classDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::StructDeclaration:
		{
			StructDeclarationSyntax* structDecl = static_cast<StructDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : structDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
			MethodSymbol* symbol = static_cast<MethodSymbol*>(semanticModel.Table->LookupSymbol(method));

			if (!module->BindMethod(symbol))
				diagnostics.ReportError(method->IdentifierToken, L"Unexpected method \'" + symbol->FullName + L"\'");

			break;
		}
	}
}

void FrameworkLoader::LoadSingleModule(FrameworkModule* module, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	SyntaxTree tree;
	{
		StringStreamReader reader = StringStreamReader(module->GetSourceCode());
		LexicalAnalyzer lexer = LexicalAnalyzer(diagnostics);
		SemanticAnalyzer semanter = SemanticAnalyzer(diagnostics);

		lexer.FromSourceReader(tree, reader);
		semanter.Analyze(tree, semanticModel);
	}

	CompilationUnitSyntax* unit = tree.CompilationUnits.at(0);
	for (MemberDeclarationSyntax* member : unit->Members)
		BindMemberDeclaration(member, module, semanticModel, diagnostics);
}
