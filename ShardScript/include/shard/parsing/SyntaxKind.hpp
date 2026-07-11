#pragma once
#include <shard/ShardScriptAPI.hpp>

namespace shard
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
		EnumDeclaration,
		EnumFieldDeclaration,

		// Members
		FieldDeclaration,
		MethodDeclaration,
		OperatorDeclaration,
		ConstructorDeclaration,
		PropertyDeclaration,
		AccessorDeclaration,
		IndexatorDeclaration,

		// Method parts
		Parameter,
		ParametersList,
		TypeParameter,
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
		ForEachStatement,
		ForInStatement,
		WhileStatement,
		UntilStatement,
		ThrowStatement,
		BreakStatement,
		ContinueStatement,
		ReturnStatement,
		IfStatement,
		UnlessStatement,
		ElseStatement,
		TryStatement,
		CatchClause,
		DeferStatement,

		// Expressions
		ObjectExpression,
		LiteralExpression,
		BinaryExpression,
		UnaryExpression,
		TernaryExpression,
		CollectionExpression,
		RangeExpression,
		IfExpression,
		SwitchExpression,
		PostfixIfExpression,
		CastExpression,
		IsExpression,

		// Linked expressions
		LinkedExpression,
		MemberAccessExpression,
		InvokationExpression,
		IndexatorExpression,
		LambdaExpression,
		TypeExpression,
		
		// Attributes
		Attribute,

		// Type identifiers
		PredefinedType,
		IdentifierNameType,
		QualifiedNameType,
		ArrayType,
		NullableType,
		GenericType,
		DelegateType,
	};
}