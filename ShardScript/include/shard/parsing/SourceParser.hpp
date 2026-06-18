#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <memory>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/AttributeSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>

#include <initializer_list>
#include <vector>

namespace shard
{
	// Note that this parser is only capable of contextual parsing, and only should be used to parse full compulation units. DO NOT try to parse individual members or expression with this parser out of stream
	class SHARD_API SourceParser
	{
	private:
		DiagnosticsContext& Diagnostics;

	public:
		SourceParser(DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceProvider(SyntaxTree& syntaxTree, SourceProvider& reader);

		// 1. top tier components
		std::unique_ptr<CompilationUnitSyntax> ReadCompilationUnit(SourceProvider& reader);
		std::unique_ptr<UsingDirectiveSyntax> ReadUsingDirective(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<NamespaceDeclarationSyntax> ReadNamespaceDeclaration(SourceProvider& reader, SyntaxNode *const parent);

		// 2. Type declarations
		std::unique_ptr<MemberDeclarationSyntax> ReadMemberDeclaration(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ClassDeclarationSyntax> ReadClassDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<StructDeclarationSyntax> ReadStructDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<InterfaceDeclarationSyntax> ReadInterfaceDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<DelegateDeclarationSyntax> ReadDelegateDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);

		std::vector<SyntaxToken> ReadMemberModifiers(SourceProvider& reader);
		std::unique_ptr<AttributeSyntax> ReadAttribute(SourceProvider& reader, SyntaxNode *const parent);
		std::vector<std::unique_ptr<AttributeSyntax>> ReadAttributeList(SourceProvider& reader, SyntaxNode *const parent);
		void ReadTypeBody(SourceProvider& reader, TypeDeclarationSyntax *const syntax);
		//TypeDeclarationSyntax *const make_type(MemberDeclarationInfo& info, SyntaxNode *const parent);

		// 3. Type members
		std::unique_ptr<ConstructorDeclarationSyntax> ReadConstructorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<MethodDeclarationSyntax> ReadMethodDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<FieldDeclarationSyntax> ReadFieldDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<PropertyDeclarationSyntax> ReadPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<PropertyDeclarationSyntax> ReadComputedPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<IndexatorDeclarationSyntax> ReadIndexatorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent);
		std::unique_ptr<AccessorDeclarationSyntax> ReadAccessorDeclaration(SourceProvider& reader, SyntaxNode *const parent);

		// 4. Code blocks
		std::unique_ptr<StatementsBlockSyntax> ReadStatementsBlock(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<StatementSyntax> ReadStatement(SourceProvider& reader, SyntaxNode *const parent);

		// 5. Keywords and statements
		std::unique_ptr<KeywordStatementSyntax> ReadKeywordStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ReturnStatementSyntax> ReadReturnStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ThrowStatementSyntax> ReadThrowStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<BreakStatementSyntax> ReadBreakStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ContinueStatementSyntax> ReadContinueStatement(SourceProvider& reader, SyntaxNode *const parent);

		// 6. Lexical structures
		std::unique_ptr<ConditionalClauseBaseSyntax> ReadConditionalClause(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<WhileStatementSyntax> ReadWhileStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<UntilStatementSyntax> ReadUntilStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ForStatementSyntax> ReadForStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ForEachStatementSyntax> ReadForEachStatement(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TryStatementSyntax> ReadTryStatement(SourceProvider& reader, SyntaxNode *const parent);

		// 7. Expression
		std::unique_ptr<ExpressionSyntax> ReadExpression(SourceProvider& reader, SyntaxNode *const parent, int bindingPower);
		std::unique_ptr<ExpressionSyntax> ReadNullDenotation(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ExpressionSyntax> ReadLeftDenotation(SourceProvider& reader, SyntaxNode *const parent, std::unique_ptr<ExpressionSyntax> leftExpr);

		std::unique_ptr<CollectionExpressionSyntax> ReadCollectionExpression(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ObjectExpressionSyntax> ReadObjectExpression(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TernaryExpressionSyntax> ReadTernaryExpression(SourceProvider& reader, std::unique_ptr<ExpressionSyntax> condition, SyntaxNode *const parent);
		std::unique_ptr<IfExpressionSyntax> ReadIfExpression(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<SwitchExpressionSyntax> ReadSwitchExpression(SourceProvider& reader, SyntaxNode *const parent);

		std::unique_ptr<LambdaExpressionSyntax> ReadLambdaExpression(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<LinkedExpressionNode> ReadLinkedExpressionNode(SourceProvider& reader, SyntaxNode *const parent, std::unique_ptr<ExpressionSyntax> lastNode, bool isFirst);
		std::unique_ptr<IndexatorExpressionSyntax> ReadIndexatorExpressionNode(SourceProvider& reader, SyntaxNode* const parent, std::unique_ptr<ExpressionSyntax> lastNode, bool isFirst);

		std::unique_ptr<ArgumentsListSyntax> ReadArgumentsList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<IndexatorListSyntax> ReadIndexatorList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ParametersListSyntax> ReadIndexerParametersList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ParametersListSyntax> ReadParametersList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<ParametersListSyntax> ReadDelegateParametersList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TypeParametersListSyntax> ReadTypeParametersList(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TypeArgumentsListSyntax> ReadTypeArgumentsList(SourceProvider& reader, SyntaxNode *const parent);

		// 9. Other
		std::vector<std::unique_ptr<TypeSyntax>> ReadBaseInterfacesList(SourceProvider& reader, SyntaxNode *const parent);

		std::unique_ptr<TypeSyntax> ReadType(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TypeSyntax> ReadIdentifierNameType(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TypeSyntax> ReadDelegateType(SourceProvider& reader, SyntaxNode *const parent);
		std::unique_ptr<TypeSyntax> ReadModifiedType(SourceProvider& reader, TypeSyntax *const type, SyntaxNode *const parent);
		std::unique_ptr<TypeSyntax> ReadArrayType(SourceProvider& reader, TypeSyntax *const type, SyntaxNode *const parent);
		std::unique_ptr<TypeSyntax> ReadGenericType(SourceProvider& reader, TypeSyntax *const previous, SyntaxNode *const parent);

	private:
		// 10. Helpers
		SyntaxToken Expect(SourceProvider& reader, TokenType kind, const wchar_t* message);
		bool Matches(SourceProvider& reader, std::initializer_list<TokenType> types);
		bool TryMatch(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
		bool TryMatchIdentifier(SourceProvider& reader, int maxSkips = 5);
	};
}
