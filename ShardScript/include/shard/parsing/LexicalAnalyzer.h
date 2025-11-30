#pragma once
#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>

#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>

#include <initializer_list>
#include <vector>

namespace shard::parsing
{
	class LexicalAnalyzer
	{
	private:
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

	public:
		LexicalAnalyzer(shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceReader(shard::parsing::lexical::SyntaxTree & syntaxTree, shard::parsing::SourceReader& reader);

		// First layer - top tier compilation
		shard::syntax::nodes::CompilationUnitSyntax* ReadCompilationUnit(shard::parsing::SourceReader& reader);

		shard::syntax::nodes::UsingDirectiveSyntax* ReadUsingDirective(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ImportDirectiveSyntax* ReadImportDirective(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		
		// Second layer - objects and members
		shard::syntax::nodes::NamespaceDeclarationSyntax* ReadNamespaceDeclaration(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::MemberDeclarationSyntax* ReadMemberDeclaration(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::ConstructorDeclarationSyntax* ReadConstructorDeclaration(shard::parsing::SourceReader& reader, shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::MethodDeclarationSyntax* ReadMethodDeclaration(shard::parsing::SourceReader& reader, shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::FieldDeclarationSyntax* ReadFieldDeclaration(shard::parsing::SourceReader& reader, shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::DelegateDeclarationSyntax* ReadDelegateDeclaration(shard::parsing::SourceReader& reader, shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::PropertyDeclarationSyntax* ReadPropertyDeclaration(shard::parsing::SourceReader& reader, shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::AccessorDeclarationSyntax* ReadAccessorDeclaration(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		std::vector<shard::syntax::SyntaxToken> ReadMemberModifiers(shard::parsing::SourceReader& reader);
		shard::syntax::nodes::ParametersListSyntax* ReadParametersList(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		void ReadTypeBody(shard::parsing::SourceReader& reader, shard::syntax::nodes::TypeDeclarationSyntax* syntax);

		// Third layer - code
		shard::syntax::nodes::StatementsBlockSyntax* ReadStatementsBlock(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::StatementSyntax* ReadStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::KeywordStatementSyntax* ReadKeywordStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ReturnStatementSyntax* ReadReturnStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ThrowStatementSyntax* ReadThrowStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::BreakStatementSyntax* ReadBreakStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ContinueStatementSyntax* ReadContinueStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::ConditionalClauseBaseSyntax* ReadConditionalClause(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::WhileStatementSyntax* ReadWhileStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::UntilStatementSyntax* ReadUntilStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ForStatementSyntax* ReadForStatement(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::ExpressionSyntax* ReadExpression(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent, int bindingPower);
		shard::syntax::nodes::ExpressionSyntax* ReadNullDenotation(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ExpressionSyntax* ReadLeftDenotation(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent, shard::syntax::nodes::ExpressionSyntax* leftExpr);

		shard::syntax::nodes::CollectionExpressionSyntax* ReadCollectionExpression(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::ObjectExpressionSyntax* ReadObjectExpression(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TernaryExpressionSyntax* ReadTernaryExpression(shard::parsing::SourceReader& reader, shard::syntax::nodes::ExpressionSyntax* condition, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::LambdaExpressionSyntax* ReadLambdaExpression(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::LinkedExpressionNode* ReadLinkedExpressionNode(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent, shard::syntax::nodes::ExpressionSyntax* lastNode, bool isFirst);

		shard::syntax::nodes::ArgumentsListSyntax* ReadArgumentsList(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::IndexatorListSyntax* ReadIndexatorList(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);

		shard::syntax::nodes::TypeSyntax* ReadType(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TypeSyntax* ReadIdentifierNameType(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TypeSyntax* ReadDelegateType(shard::parsing::SourceReader& reader, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TypeSyntax* ReadModifiedType(shard::parsing::SourceReader& reader, shard::syntax::nodes::TypeSyntax* type, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TypeSyntax* ReadArrayType(shard::parsing::SourceReader& reader, shard::syntax::nodes::TypeSyntax* type, shard::syntax::SyntaxNode* parent);
		shard::syntax::nodes::TypeSyntax* ReadGenericType(shard::parsing::SourceReader& reader, shard::syntax::nodes::TypeSyntax* previous, shard::syntax::SyntaxNode* parent);

	private:
		// Fourth layer - lexing helpers
		shard::syntax::SyntaxToken Expect(shard::parsing::SourceReader& reader, shard::syntax::TokenType kind, const wchar_t* message);
		bool Matches(shard::parsing::SourceReader& reader, std::initializer_list<shard::syntax::TokenType> types);
		bool TryMatch(shard::parsing::SourceReader& reader, std::initializer_list<shard::syntax::TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
		shard::syntax::nodes::TypeDeclarationSyntax* make_type(shard::parsing::lexical::MemberDeclarationInfo& info, shard::syntax::SyntaxNode* parent);
	};
}
