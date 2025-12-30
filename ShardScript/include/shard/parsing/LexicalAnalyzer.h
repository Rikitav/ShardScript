#pragma once
#include <shard/ShardScriptAPI.h>

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
#include <shard/syntax/nodes/TypeParametersListSyntax.h>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.h>
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

namespace shard
{
	class SHARD_API LexicalAnalyzer
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		LexicalAnalyzer(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceReader(shard::SyntaxTree & syntaxTree, shard::SourceReader& reader);

		// First layer - top tier compilation
		shard::CompilationUnitSyntax* ReadCompilationUnit(shard::SourceReader& reader);

		shard::UsingDirectiveSyntax* ReadUsingDirective(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ImportDirectiveSyntax* ReadImportDirective(shard::SourceReader& reader, shard::SyntaxNode* parent);
		
		// Second layer - objects and members
		shard::NamespaceDeclarationSyntax* ReadNamespaceDeclaration(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::MemberDeclarationSyntax* ReadMemberDeclaration(shard::SourceReader& reader, shard::SyntaxNode* parent);

		shard::ConstructorDeclarationSyntax* ReadConstructorDeclaration(shard::SourceReader& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::MethodDeclarationSyntax* ReadMethodDeclaration(shard::SourceReader& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::FieldDeclarationSyntax* ReadFieldDeclaration(shard::SourceReader& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::DelegateDeclarationSyntax* ReadDelegateDeclaration(shard::SourceReader& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::PropertyDeclarationSyntax* ReadPropertyDeclaration(shard::SourceReader& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::AccessorDeclarationSyntax* ReadAccessorDeclaration(shard::SourceReader& reader, shard::SyntaxNode* parent);

		std::vector<shard::SyntaxToken> ReadMemberModifiers(shard::SourceReader& reader);
		void ReadTypeBody(shard::SourceReader& reader, shard::TypeDeclarationSyntax* syntax);

		// Third layer - code
		shard::StatementsBlockSyntax* ReadStatementsBlock(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::StatementSyntax* ReadStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);

		shard::KeywordStatementSyntax* ReadKeywordStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ReturnStatementSyntax* ReadReturnStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ThrowStatementSyntax* ReadThrowStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::BreakStatementSyntax* ReadBreakStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ContinueStatementSyntax* ReadContinueStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);

		shard::ConditionalClauseBaseSyntax* ReadConditionalClause(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::WhileStatementSyntax* ReadWhileStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::UntilStatementSyntax* ReadUntilStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ForStatementSyntax* ReadForStatement(shard::SourceReader& reader, shard::SyntaxNode* parent);

		shard::ExpressionSyntax* ReadExpression(shard::SourceReader& reader, shard::SyntaxNode* parent, int bindingPower);
		shard::ExpressionSyntax* ReadNullDenotation(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ExpressionSyntax* ReadLeftDenotation(shard::SourceReader& reader, shard::SyntaxNode* parent, shard::ExpressionSyntax* leftExpr);

		shard::CollectionExpressionSyntax* ReadCollectionExpression(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ObjectExpressionSyntax* ReadObjectExpression(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TernaryExpressionSyntax* ReadTernaryExpression(shard::SourceReader& reader, shard::ExpressionSyntax* condition, shard::SyntaxNode* parent);

		shard::LambdaExpressionSyntax* ReadLambdaExpression(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::LinkedExpressionNode* ReadLinkedExpressionNode(shard::SourceReader& reader, shard::SyntaxNode* parent, shard::ExpressionSyntax* lastNode, bool isFirst);

		shard::ArgumentsListSyntax* ReadArgumentsList(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::IndexatorListSyntax* ReadIndexatorList(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::ParametersListSyntax* ReadParametersList(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TypeParametersListSyntax* ReadTypeParametersList(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TypeArgumentsListSyntax* ReadTypeArgumentsList(shard::SourceReader& reader, shard::SyntaxNode* parent);

		shard::TypeSyntax* ReadType(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadIdentifierNameType(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadDelegateType(shard::SourceReader& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadModifiedType(shard::SourceReader& reader, shard::TypeSyntax* type, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadArrayType(shard::SourceReader& reader, shard::TypeSyntax* type, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadGenericType(shard::SourceReader& reader, shard::TypeSyntax* previous, shard::SyntaxNode* parent);

	private:
		// Fourth layer - lexing helpers
		shard::SyntaxToken Expect(shard::SourceReader& reader, shard::TokenType kind, const wchar_t* message);
		bool Matches(shard::SourceReader& reader, std::initializer_list<shard::TokenType> types);
		bool TryMatch(shard::SourceReader& reader, std::initializer_list<shard::TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
		shard::TypeDeclarationSyntax* make_type(shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
	};
}
