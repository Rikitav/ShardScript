#pragma once

namespace shard::syntax
{
	enum class SyntaxKind
	{
		Unknown,

		CompilationUnit,
		UsingDirective,
		DllImportDirective,

		NamespaceDeclaration,
		ClassDeclaration,
		StructDeclaration,
		InterfaceDeclaration,

		MethodDeclaration,
		ParametersList,
		Parameter,
		ArgumentsList,
		Argument,
		StatementsBlock,
		MethodBody,

		Statement,
		KeywordStatement,
		ExpressionStatement,
		VariableStatement,
		//BlockStatement,
		ForStatementSyntax,

		Expression,
		AssignExpression,
		ConstExpression,
		BinaryExpression,
		UnaryExpression,
		MemberAccessExpression,
		InvokationExpression,
		
		Type
	};
}