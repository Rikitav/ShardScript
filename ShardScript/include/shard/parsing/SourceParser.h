#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/lexical/SourceProvider.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/MemberDeclarationInfo.h>
#include <shard/parsing/SyntaxTree.h>

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

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h>

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
	class SHARD_API SourceParser
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		SourceParser(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceProvider(shard::SyntaxTree& syntaxTree, shard::SourceProvider& reader);

		// 1. top tier components
		shard::CompilationUnitSyntax* ReadCompilationUnit(shard::SourceProvider& reader);
		shard::UsingDirectiveSyntax* ReadUsingDirective(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::NamespaceDeclarationSyntax* ReadNamespaceDeclaration(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 2. Type declarations
		shard::MemberDeclarationSyntax* ReadMemberDeclaration(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ClassDeclarationSyntax* ReadClassDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::StructDeclarationSyntax* ReadStructDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		//shard::InterfaceDelcarationSyntax* ReadInterfaceDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::DelegateDeclarationSyntax* ReadDelegateDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);

		std::vector<shard::SyntaxToken> ReadMemberModifiers(shard::SourceProvider& reader);
		void ReadTypeBody(shard::SourceProvider& reader, shard::TypeDeclarationSyntax* syntax);
		//shard::TypeDeclarationSyntax* make_type(shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);

		// 3. Type members
		shard::ConstructorDeclarationSyntax* ReadConstructorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::MethodDeclarationSyntax* ReadMethodDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::FieldDeclarationSyntax* ReadFieldDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::PropertyDeclarationSyntax* ReadPropertyDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::IndexatorDeclarationSyntax* ReadIndexatorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode* parent);
		shard::AccessorDeclarationSyntax* ReadAccessorDeclaration(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 4. Code blocks
		shard::StatementsBlockSyntax* ReadStatementsBlock(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::StatementSyntax* ReadStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 5. Keywords and statements
		shard::KeywordStatementSyntax* ReadKeywordStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ReturnStatementSyntax* ReadReturnStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ThrowStatementSyntax* ReadThrowStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::BreakStatementSyntax* ReadBreakStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ContinueStatementSyntax* ReadContinueStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 6. Lexical structures
		shard::ConditionalClauseBaseSyntax* ReadConditionalClause(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::WhileStatementSyntax* ReadWhileStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::UntilStatementSyntax* ReadUntilStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ForStatementSyntax* ReadForStatement(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 7. Expression
		shard::ExpressionSyntax* ReadExpression(shard::SourceProvider& reader, shard::SyntaxNode* parent, int bindingPower);
		shard::ExpressionSyntax* ReadNullDenotation(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ExpressionSyntax* ReadLeftDenotation(shard::SourceProvider& reader, shard::SyntaxNode* parent, shard::ExpressionSyntax* leftExpr);

		shard::CollectionExpressionSyntax* ReadCollectionExpression(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ObjectExpressionSyntax* ReadObjectExpression(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TernaryExpressionSyntax* ReadTernaryExpression(shard::SourceProvider& reader, shard::ExpressionSyntax* condition, shard::SyntaxNode* parent);

		shard::LambdaExpressionSyntax* ReadLambdaExpression(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::LinkedExpressionNode* ReadLinkedExpressionNode(shard::SourceProvider& reader, shard::SyntaxNode* parent, shard::ExpressionSyntax* lastNode, bool isFirst);

		shard::ArgumentsListSyntax* ReadArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::IndexatorListSyntax* ReadIndexatorList(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ParametersListSyntax* ReadIndexerParametersList(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::ParametersListSyntax* ReadParametersList(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TypeParametersListSyntax* ReadTypeParametersList(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TypeArgumentsListSyntax* ReadTypeArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode* parent);

		// 8. Other
		shard::TypeSyntax* ReadType(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadIdentifierNameType(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadDelegateType(shard::SourceProvider& reader, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadModifiedType(shard::SourceProvider& reader, shard::TypeSyntax* type, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadArrayType(shard::SourceProvider& reader, shard::TypeSyntax* type, shard::SyntaxNode* parent);
		shard::TypeSyntax* ReadGenericType(shard::SourceProvider& reader, shard::TypeSyntax* previous, shard::SyntaxNode* parent);

	private:
		// Fourth layer - lexing helpers
		shard::SyntaxToken Expect(shard::SourceProvider& reader, shard::TokenType kind, const wchar_t* message);
		bool Matches(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types);
		bool TryMatch(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
	};
}
