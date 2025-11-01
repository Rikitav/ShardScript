#pragma once

namespace shard::syntax
{
	enum class SyntaxKind
	{
		Unknown,

		// Top-tier Units
		CompilationUnit,
		UsingDirective,
		DllImportDirective,

		// Types
		NamespaceDeclaration,
		ClassDeclaration,
		StructDeclaration,
		InterfaceDeclaration,

		// Members
		FieldDeclaration,
		MethodDeclaration,

		// Method parts
		Parameter,
		ParametersList,
		Argument,
		ArgumentsList,
		IndexatorList,
		StatementsBlock,
		MethodBody,

		// Statements
		ExpressionStatement,
		VariableStatement,

		// Keyword statements
		ForStatement,
		WhileStatement,
		UntilStatement,
		ReturnStatement,
		IfStatement,
		UnlessStatement,
		ElseStatement,

		// Expressions
		ObjectExpression,
		LiteralExpression,
		BinaryExpression,
		UnaryExpression,

		// Linked expressions
		LinkedExpression,
		MemberAccessExpression,
		InvokationExpression,
		IndexatorExpression,
		
		// Type identifiers
		PredefinedType,
		IdentifierNameType,
		ArrayType,
		NullableType,
		GenericType,
	};
}