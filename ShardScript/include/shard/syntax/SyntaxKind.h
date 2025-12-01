#pragma once
#include <shard/ShardScriptAPI.h>

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
		DelegateDeclaration,

		// Members
		FieldDeclaration,
		MethodDeclaration,
		ConstructorDeclaration,
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
		TernaryExpression,
		CollectionExpression,

		// Linked expressions
		LinkedExpression,
		MemberAccessExpression,
		InvokationExpression,
		IndexatorExpression,
		LambdaExpression,
		
		// Type identifiers
		PredefinedType,
		IdentifierNameType,
		ArrayType,
		NullableType,
		GenericType,
		DelegateType,
	};
}