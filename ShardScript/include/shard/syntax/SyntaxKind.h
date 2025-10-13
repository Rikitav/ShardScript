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
		Block,
		MethodBody,

		Statement,
		KeywordStatement,
		ExpressionStatement,
		VariableStatement,
		BlockStatement,

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