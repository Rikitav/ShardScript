#pragma once

namespace shard::syntax
{
	enum class TokenType
	{
		// Generic
		Unknown,
		EndOfFile,   // End of file
		Trivia,		 // any non visible
		VarName,	 // variable name
		Identifier,	 // member name

		// Binary arithmetic operators
		AddOperator,		  // +
		SubOperator,		  // -
		MultOperator,		  // *
		DivOperator,		  // /
		ModOperator,		  // %
		PowOperator,	      // ^
		AddAssignOperator,    // +=
		SubAssignOperator,    // -=
		MultAssignOperator,   // *=
		DivAssignOperator,    // /=
		ModAssignOperator,    // %=
		PowAssignOperator,    // ^=

		// Binary boolean operators
		EqualsOperator,		     // ==
		NotEqualsOperator,	     // !=
		GreaterOperator,		 // >
		GreaterOrEqualsOperator, // >=
		LessOperator,		     // <
		LessOrEqualsOperator,    // <=
		AssignOperator,          // =

		// Unary arithmetic operators
		IncrementOperator,   // ++
		DecrementOperator,   // --

		// Unary boolean operators
		NotOperator,         // !

		// Literals
		NullLiteral,          // 'null'
		StringLiteral,		  // Value in ""
		BooleanLiteral,		  // 'true' or 'false'
		NumberLiteral,	      // Number value
		NativeLiteral,		  // native pointer

		// Punctuation
		Semicolon,			  // ;
		OpenBrace,			  // {
		CloseBrace,  		  // }
		OpenCurl,			  // (
		CloseCurl,			  // )
		OpenSquare,			  // [
		CloseSquare,		  // ]
		Delimeter,			  // .
		Comma,				  // ,

		// Member modifier keywords
		PublicKeyword,		  // public
		PrivateKeyword,		  // private
		ProtectedKeyword,	  // protected
		InternalKeyword,	  // internal
		StaticKeyword,		  // static
		AbstractKeyword,	  // abstract
		SealedKeyword,		  // sealed
		PartialKeyword,		  // partial

		// Built-in type keywords
		VoidKeyword,		  // void
		IntegerKeyword,		  // int
		ShortKeyword,		  // short
		LongKeyword,		  // long
		StringKeyword,		  // string
		BooleanKeyword,		  // bool

		// Directive declaration keywords
		UsingKeyword,		  // using
		FromKeyword,		  // from
		ImportKeyword,		  // import

		// Type declaration keywords
		MethodKeyword,		  // method
		ClassKeyword,		  // class
		StructKeyword,		  // struct
		InterfaceKeyword,	  // interface
		NamespaceKeyword,	  // namespace

		// Loops keywords
		ForKeyword,
		WhileKeyword,
		DoKeyword,
		ForeachKeyword,
		ForeverKeyword,

		// Conditional keywords
		IfKeyword,
		UnlessKeyword,
		ElseKeyword,

		// Functional keywords
		ReturnKeyword,		  // return
		BreakKeyword,		  // break
		ContinueKeyword,	  // continue
	};

	struct TokenTypeHash
	{
		size_t operator ()(const TokenType& type) const {
			return static_cast<size_t>(type);
		}
	};
}