#pragma once

namespace shard
{
	enum class SyntaxKind
	{
		Unknown,

		// Top-tier Units
		CompilationUnit,
		UsingDirective,
		NamespaceDirective,

		// Types
		ClassDeclaration,
		StructDeclaration,
		InterfaceDeclaration,
		DelegateDeclaration,
		EnumDeclaration,
		EnumFieldDeclaration,

		// Members
		FieldDeclaration,
		FunctionDeclaration,
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
		TypeArgumentsList,
		TypeParametersList,
		BaseTypesList,
		StatementsBlock,
		ArrowClause,
		WhereClause,
		WhereClausesList,

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
		SwitchStatement,
		SwitchCaseClause,

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
		IsPattern,

		// Linked expressions
		LinkedExpression,
		MemberAccessExpression,
		AwaitExpression,
		InvokationExpression,
		IndexatorExpression,
		LambdaExpression,
		TypeExpression,

		// Attributes
		Attribute,
		AttributesList,

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