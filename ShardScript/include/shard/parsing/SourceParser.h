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
	// Note that this parser is only capable of contextual parsing, and only should be used to parse full compulation units. DO NOT try to parse individual members or expression with this parser out of stream
	class SHARD_API SourceParser
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		SourceParser(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceProvider(shard::SyntaxTree& syntaxTree, shard::SourceProvider& reader);

		// 1. top tier components
		shard::CompilationUnitSyntax *const ReadCompilationUnit(shard::SourceProvider& reader);
		shard::UsingDirectiveSyntax *const ReadUsingDirective(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::NamespaceDeclarationSyntax *const ReadNamespaceDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 2. Type declarations
		shard::MemberDeclarationSyntax *const ReadMemberDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ClassDeclarationSyntax *const ReadClassDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::StructDeclarationSyntax *const ReadStructDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		//shard::InterfaceDelcarationSyntax *const ReadInterfaceDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::DelegateDeclarationSyntax *const ReadDelegateDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);

		std::vector<shard::SyntaxToken> ReadMemberModifiers(shard::SourceProvider& reader);
		void ReadTypeBody(shard::SourceProvider& reader, shard::TypeDeclarationSyntax *const syntax);
		//shard::TypeDeclarationSyntax *const make_type(shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);

		// 3. Type members
		shard::ConstructorDeclarationSyntax *const ReadConstructorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::MethodDeclarationSyntax *const ReadMethodDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::FieldDeclarationSyntax *const ReadFieldDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::PropertyDeclarationSyntax *const ReadPropertyDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::IndexatorDeclarationSyntax *const ReadIndexatorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::AccessorDeclarationSyntax *const ReadAccessorDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 4. Code blocks
		shard::StatementsBlockSyntax *const ReadStatementsBlock(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::StatementSyntax *const ReadStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 5. Keywords and statements
		shard::KeywordStatementSyntax *const ReadKeywordStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ReturnStatementSyntax *const ReadReturnStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ThrowStatementSyntax *const ReadThrowStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::BreakStatementSyntax *const ReadBreakStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ContinueStatementSyntax *const ReadContinueStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 6. Lexical structures
		shard::ConditionalClauseBaseSyntax *const ReadConditionalClause(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::WhileStatementSyntax *const ReadWhileStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::UntilStatementSyntax *const ReadUntilStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ForStatementSyntax *const ReadForStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 7. Expression
		shard::ExpressionSyntax *const ReadExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent, int bindingPower);
		shard::ExpressionSyntax *const ReadNullDenotation(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ExpressionSyntax *const ReadLeftDenotation(shard::SourceProvider& reader, shard::SyntaxNode *const parent, shard::ExpressionSyntax *const leftExpr);

		shard::CollectionExpressionSyntax *const ReadCollectionExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ObjectExpressionSyntax *const ReadObjectExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TernaryExpressionSyntax *const ReadTernaryExpression(shard::SourceProvider& reader, shard::ExpressionSyntax *const condition, shard::SyntaxNode *const parent);

		shard::LambdaExpressionSyntax *const ReadLambdaExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::LinkedExpressionNode *const ReadLinkedExpressionNode(shard::SourceProvider& reader, shard::SyntaxNode *const parent, shard::ExpressionSyntax *const lastNode, bool isFirst);
		shard::IndexatorExpressionSyntax* const ReadIndexatorExpressionNode(shard::SourceProvider& reader, shard::SyntaxNode* const parent, shard::ExpressionSyntax* const lastNode, bool isFirst);

		shard::ArgumentsListSyntax *const ReadArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::IndexatorListSyntax *const ReadIndexatorList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ParametersListSyntax *const ReadIndexerParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ParametersListSyntax *const ReadParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeParametersListSyntax *const ReadTypeParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeArgumentsListSyntax *const ReadTypeArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 8. Other
		shard::TypeSyntax *const ReadType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadIdentifierNameType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadDelegateType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadModifiedType(shard::SourceProvider& reader, shard::TypeSyntax *const type, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadArrayType(shard::SourceProvider& reader, shard::TypeSyntax *const type, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadGenericType(shard::SourceProvider& reader, shard::TypeSyntax *const previous, shard::SyntaxNode *const parent);

	private:
		// Fourth layer - lexing helpers
		shard::SyntaxToken Expect(shard::SourceProvider& reader, shard::TokenType kind, const wchar_t* message);
		bool Matches(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types);
		bool TryMatch(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
	};
}
