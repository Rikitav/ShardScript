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
		IndexatorList,
		Argument,
		StatementsBlock,
		MethodBody,

		Statement,
		KeywordStatement,
		ExpressionStatement,
		VariableStatement,
		ForStatement,
		ReturnStatement,

		Expression,
		AssignExpression,
		ConstExpression,
		BinaryExpression,
		UnaryExpression,
		FieldAccessExpression,
		InvokationExpression,
		IndexatorExpression,
		
		Type
	};
}