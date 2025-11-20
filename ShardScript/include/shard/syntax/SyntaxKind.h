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
		PropertyDeclaration,
		AccessorDeclaration,

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
		ThrowStatement,
		BreakStatement,
		ContinueStatement,
		ReturnStatement,
		IfStatement,
		UnlessStatement,
		ElseStatement,

		// Expressions
		ObjectExpression,
		LiteralExpression,
		BinaryExpression,
		UnaryExpression,
		CollectionExpression,

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