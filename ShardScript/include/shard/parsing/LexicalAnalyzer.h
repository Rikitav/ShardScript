#pragma once
#include <shard/parsing/SourceReader.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>

#include <shard/syntax/nodes/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MethodBodySyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/TypeDeclarations.h>
#include <shard/syntax/nodes/Statements.h>

#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <vector>

using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::analysis;
using namespace shard::syntax::structures;
using namespace shard::parsing::structures;

namespace shard::parsing
{
	class LexicalAnalyzer
	{
	private:
		shared_ptr<SyntaxTree> Tree;
		DiagnosticsContext& Diagnostics;
		//shared_ptr<CompilationUnitSyntax> CompilationUnit;

	public:
		LexicalAnalyzer(shared_ptr<SyntaxTree> tree, DiagnosticsContext& diagnostics);

		void FromSourceReader(SourceReader& Reader);

		// First layer - top tier compilation
		shared_ptr<CompilationUnitSyntax> ReadCompilationUnit(SourceReader& reader);

		shared_ptr<UsingDirectiveSyntax> ReadUsingDirective(SourceReader& reader);
		
		// Second layer - objects and members
		shared_ptr<NamespaceDeclarationSyntax> ReadNamespaceDeclaration(SourceReader& reader);
		shared_ptr<MemberDeclarationSyntax> ReadMemberDeclaration(SourceReader& reader);

		vector<SyntaxToken> ReadMemberModifiers(SourceReader& reader);
		shared_ptr<ParametersListSyntax> ReadParametersList(SourceReader& reader);
		void ReadTypeBody(SourceReader& reader, shared_ptr<TypeDeclarationSyntax> syntax);

		// Third layer - code
		shared_ptr<MethodBodySyntax> ReadMethodBody(SourceReader& reader);
		shared_ptr<StatementSyntax> ReadStatement(SourceReader& reader);
		shared_ptr<KeywordStatementSyntax> ReadKeywordStatement(SourceReader& reader);

		shared_ptr<ExpressionSyntax> ReadExpression(SourceReader& reader, int bindingPower);
		shared_ptr<ExpressionSyntax> ReadNullDenotation(SourceReader& reader);
		shared_ptr<ExpressionSyntax> ReadLeftDenotation(SourceReader& reader, shared_ptr<ExpressionSyntax> leftExpr);

		shared_ptr<ExpressionSyntax> ReadMemberAccessExpression(SourceReader& reader);
		shared_ptr<ArgumentsListSyntax> ReadArgumentsList(SourceReader& reader);

	private:
		// Fourth layer - lexing helpers
		SyntaxToken Expect(SourceReader& reader, TokenType kind, const char* message)
		{
			SyntaxToken current = reader.Current();
			if (current.Type == kind)
			{
				reader.Consume();
				return current;
			}

			if (message != nullptr)
				Diagnostics.ReportError(current, message);
			
			return SyntaxToken(kind, "", current.Location, true);
		}

		bool Matches(SourceReader& reader, initializer_list<TokenType> types)
		{
			if (!reader.CanConsume())
				return false;

			SyntaxToken current = reader.Current();
			for (const TokenType& type : types)
			{
				if (current.Type == type)
					return true;
			}

			return false;
		}

		shared_ptr<TypeDeclarationSyntax> make_type(MemberDeclarationInfo& info)
		{
			if (info.DeclareType.IsMissing)
				throw runtime_error("declare type is missing");

			switch (info.DeclareType.Type)
			{
				case TokenType::ClassKeyword:
					return make_shared<ClassDeclarationSyntax>(info);
				
				default:
					throw runtime_error("unknown type delcaration");
			}
		}
	};
}
