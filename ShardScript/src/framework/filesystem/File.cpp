#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/reading/FileReader.h>
#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/visiting/TypeBinder.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>

#include <shlwapi.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <filesystem>

#pragma comment(lib, "shlwapi.lib")

using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::semantic;
using namespace shard::parsing::lexical;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

static const std::wstring Framework_Class_File =
L"                                                                                \
namespace System																  \
{																				  \
	public static class File													  \
	{																			  \
		public static string ReadAllText(string fileName);						  \
		public static string WriteAllText(string fileName, string content);		  \
   }																			  \
}																				  \
";

static ObjectInstance* Impl_ReadAllText(InboundVariablesContext* arguments)
{
	std::wstring fileName = arguments->Variables.at(L"fileName")->ReadPrimitive<std::wstring>();
	std::wifstream fileStream(fileName);

	if (!fileStream.is_open())
		throw std::runtime_error("Failed to open text file.");

	std::wstring content = std::wstring(std::istreambuf_iterator<wchar_t>(fileStream), std::istreambuf_iterator<wchar_t>());
 	return ObjectInstance::FromValue(content);
}

static ObjectInstance* Impl_WriteAllText(InboundVariablesContext* arguments)
{
	std::wstring fileName = arguments->Variables.at(L"fileName")->ReadPrimitive<std::wstring>();
	std::wstring content = arguments->Variables.at(L"content")->ReadPrimitive<std::wstring>();
	std::wofstream fileStream(fileName);

	if (!fileStream.is_open())
		throw std::runtime_error("Failed to open text file.");
	
	fileStream.write(content.c_str(), content.size());

	if (fileStream.fail())
		throw std::runtime_error("File writing failed.");

	return nullptr;
}

static void LoadModule_File(LexicalAnalyzer& lexer, SemanticModel& model, DiagnosticsContext& diagnostics)
{
	SyntaxTree tree;
	StringStreamReader reader = StringStreamReader(Framework_Class_File);
	lexer.FromSourceReader(tree, reader);

	DeclarationCollector collector(model.Table, diagnostics);
	collector.VisitSyntaxTree(tree);

	TypeBinder binder(model.Table, diagnostics);
	binder.VisitSyntaxTree(tree);

	NamespaceDeclarationSyntax* fileNamespace = static_cast<NamespaceDeclarationSyntax*>(tree.CompilationUnits.at(0)->Members.at(0));
	ClassDeclarationSyntax* fileClass = static_cast<ClassDeclarationSyntax*>(fileNamespace->Members.at(0));

	for (MemberDeclarationSyntax* member : fileClass->Members)
	{
		switch (member->Kind)
		{
			case SyntaxKind::MethodDeclaration:
			{
				MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
				if (method->IdentifierToken.Word == L"ReadAllText")
				{
					MethodSymbol* symbol = static_cast<MethodSymbol*>(model.Table->LookupSymbol(method));
					symbol->HandleType = MethodHandleType::FunctionPointer;
					symbol->FunctionPointer = Impl_ReadAllText;
				}
				else if (method->IdentifierToken.Word == L"WriteAllText")
				{
					MethodSymbol* symbol = static_cast<MethodSymbol*>(model.Table->LookupSymbol(method));
					symbol->HandleType = MethodHandleType::FunctionPointer;
					symbol->FunctionPointer = Impl_WriteAllText;
				}
				else
				{
					diagnostics.ReportError(method->IdentifierToken, L"Unexpected method in System.File class loader");
				}

				break;
			}
		}
	}
}