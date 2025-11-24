#pragma once

namespace shard::syntax
{
	enum class TokenType
	{
		// Generic
		Unknown,
		EndOfFile,   // End of file
		//Trivia,    // any non visible
		//VarName,	 // variable name
		NewKeyword,  // new keyword
		Identifier,	 // member name

		// Generic operators
		AssignOperator,     // =

		// Binary arithmetic operators
		AddOperator,		  // +
		SubOperator,		  // -
		MultOperator,		  // *
		DivOperator,		  // /
		ModOperator,		  // %
		PowOperator,	      // ^

		// Assigning binary arithmetic operator
		AddAssignOperator,    // +=
		SubAssignOperator,    // -=
		MultAssignOperator,   // *=
		DivAssignOperator,    // /=
		ModAssignOperator,    // %=
		PowAssignOperator,    // ^=

		// Unary arithmetic operators
		IncrementOperator,   // ++
		DecrementOperator,   // --

		// Binary bitwise operators
		OrOperator,				// |
		AndOperator,			// &
		RightShiftOperator,		// >>
		LeftShiftOperator,		// <<

		// Assigning binary bitwise operators
		OrAssignOperator,			// |=
		AndAssignOperator,			// &=
		//RightShiftAssignOperator,	// >>=
		//LeftShiftAssignOperator,	// <<=

		// Binary boolean-returning operators
		EqualsOperator,		     // ==
		NotEqualsOperator,	     // !=
		GreaterOperator,		 // >
		GreaterOrEqualsOperator, // >=
		LessOperator,		     // <
		LessOrEqualsOperator,    // <=

		// Unary boolean operators
		NotOperator,         // !

		// Literals
		NullLiteral,          // 'null'
		CharLiteral,		  // single character in ''
		StringLiteral,		  // string in ""
		BooleanLiteral,		  // 'true' or 'false'
		NumberLiteral,	      // Number value
		NativeLiteral,		  // native pointer

		// Punctuation
		Colon,				  // :
		Semicolon,			  // ;
		OpenBrace,			  // {
		CloseBrace,  		  // }
		OpenCurl,			  // (
		CloseCurl,			  // )
		OpenSquare,			  // [
		CloseSquare,		  // ]
		Delimeter,			  // .
		Comma,				  // ,
		Question,			  // ?

		// Member modifier keywords
		PublicKeyword,		  // public
		PrivateKeyword,		  // private
		ProtectedKeyword,	  // protected
		InternalKeyword,	  // internal
		StaticKeyword,		  // static
		AbstractKeyword,	  // abstract
		SealedKeyword,		  // sealed
		PartialKeyword,		  // partial
		OverrideKeyword,	  // override
		VirtualKeyword,		  // virtual
		
		// Property accessor keywords
		GetKeyword,			  // get
		SetKeyword,			  // set
		FieldKeyword,		  // field

		// Built-in type keywords
		VoidKeyword,		  // void
		VarKeyword,			  // var
		IntegerKeyword,		  // int
		ShortKeyword,		  // short
		LongKeyword,		  // long
		CharKeyword,		  // char
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
		ForKeyword,			// for
		WhileKeyword,		// while
		UntilKeyword,		// until
		DoKeyword,			// do
		ForeachKeyword,		// foreach

		// Conditional clause keywords
		IfKeyword,		 // if
		UnlessKeyword,	 // unless
		ElseKeyword,	 // else

		// Functional keywords
		ReturnKeyword,		  // return
		ThrowKeyword,		  // throw
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