#pragma once

namespace shard::syntax
{
	enum class TokenType
	{
		// Generic
		Unknown,
		EndOfFile,			  // End of file
		Trivia,				  // any non visible
		VarName,			  // variable name
		Identifier,			  // member name
		Type,				  // return type of method

		// Binary arithmetic operators
		AddOperator,    // +
		SubOperator,	// -
		MultOperator,	// *
		DivOperator,	// /
		ModOperator,	// %
		PowOperator,	// ^

		// Binary boolean operators
		EqualsOperator,		     // ==
		NotEqualsOperator,	     // !=
		GreaterOperator,		 // >
		GreaterOrEqualsOperator, // >=
		LessOperator,		     // <
		LessOrEqualsOperator,    // <=
		AssignOperator,          // =

		// Unary arithmetic operators
		AddAssignOperator,    // +=
		SubAssignOperator,    // -=
		MultAssignOperator,   // *=
		DivAssignOperator,    // /=
		ModAssignOperator,    // %=
		PowAssignOperator,    // ^=

		// Literals
		StringLiteral,		  // Value in ""
		BooleanLiteral,		  // 'true' or 'false'
		NumberLiteral,	      // Number value
		NativeLiteral,		  // Pointer

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

		// Type declaration keywords
		UsingKeyword,		  // using
		MethodKeyword,		  // method
		ClassKeyword,		  // class
		StructKeyword,		  // struct
		InterfaceKeyword,	  // interface
		NamespaceKeyword,	  // namespace

		// functional keywords
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