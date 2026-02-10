// ShardScript Single Header Library
// Auto-generated. Do not edit directly.
#ifndef SHARDSCRIPT_SINGLE_HEADER_H
#define SHARDSCRIPT_SINGLE_HEADER_H

// --- Begin: shard/ShardScriptAPI.h ---

#ifdef SHARD_API_EXPORT
	#define SHARD_API __declspec(dllexport)
#else
	#define SHARD_API __declspec(dllimport)
#endif

#if _DEBUG
#pragma warning(disable: 4251)
#endif

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define SafeInit TOKENPASTE2(InitLibrary_, _MSC_VER)

extern "C" __declspec(dllexport) void SafeInit();

//#define EXPORT_EXTERN(name) ObjectInstance* &name(MethodSymbol* method, InboundVariableContext* arguments)
// --- End: shard/ShardScriptAPI.h ---

// --- Begin: shard/ShardScriptVersion.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

namespace shard
{
	class SHARD_API ShardScriptVersion
	{
	public:
		static const int Major;
		static const int Minor;

		bool IsCompatibleWith(const int major, const int minor);
	};
}
// --- End: shard/ShardScriptVersion.h ---

// --- Begin: shard/syntax/TokenType.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

namespace shard
{
	enum class TokenType
	{
		// Generic
		Unknown,
		EndOfFile,   // End of file
		Trivia,      // any non-visible
		NewKeyword,  // new keyword
		Identifier,	 // member name

		// Generic operators
		AssignOperator,     // =
		LambdaOperator,		// =>

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
		NumberLiteral,	      // number value
		DoubleLiteral,		  // floating point number value
		NativeLiteral,		  // architectures' native number

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
		ExternKeyword,		  // extern

		/*
		// OOP specific
		AbstractKeyword,	  // abstract
		SealedKeyword,		  // sealed
		PartialKeyword,		  // partial
		OverrideKeyword,	  // override
		VirtualKeyword,		  // virtual
		*/

		// Property accessor keywords
		GetKeyword,			  // get
		SetKeyword,			  // set
		FieldKeyword,		  // field
		IndexerKeyword,		  // indexer

		// Built-in type keywords
		VoidKeyword,		  // void
		VarKeyword,			  // var
		IntegerKeyword,		  // int
		DoubleKeyword,		  // double
		ShortKeyword,		  // short
		LongKeyword,		  // long
		CharKeyword,		  // char
		StringKeyword,		  // string
		BooleanKeyword,		  // bool
		DelegateKeyword,	  // delegate
		LambdaKeyword,		  // lambda

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
}
// --- End: shard/syntax/TokenType.h ---

// --- Begin: shard/parsing/analysis/TextLocation.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

#include <string>

namespace shard
{
	struct SHARD_API TextLocation
	{
	public:
		const std::wstring FileName;
		const int Line;
		const int Offset;
		const int Length;

		inline TextLocation()
			: FileName(L""), Line(0), Offset(0), Length(0) { }

		inline TextLocation(std::wstring filename, int line, int offset, int length)
			: FileName(filename), Line(line), Offset(offset), Length(length) {}

		inline TextLocation(const TextLocation& other)
			: FileName(other.FileName), Line(other.Line), Offset(other.Offset), Length(other.Length) { }
	};
}
// --- End: shard/parsing/analysis/TextLocation.h ---

// --- Begin: shard/syntax/SyntaxToken.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/TokenType.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)

#include <string>
#include <utility>

namespace shard
{
	struct SHARD_API SyntaxToken
	{
	private:
		inline static int counter = 0;

	public:
		const int Index;
		const bool IsMissing;
		const TokenType Type;
		const std::wstring Word;
		const shard::TextLocation Location;

		inline SyntaxToken()
			: Index(-1), IsMissing(true), Type(TokenType::Unknown), Word() { }

		inline SyntaxToken(const TokenType type, std::wstring word, const shard::TextLocation& location, const bool isMissing = false)
			: Index(counter++), IsMissing(isMissing), Type(type), Word(std::move(word)), Location(location) { }

		inline SyntaxToken(const SyntaxToken& other)
			: Index(other.Index), IsMissing(other.IsMissing), Type(other.Type), Word(other.Word), Location(other.Location) { }

		inline SyntaxToken& operator=(const SyntaxToken& other)
		{
			if (this != &other)
			{
				this->~SyntaxToken();
				new (this) SyntaxToken(other);
			}

			return *this;
		}
	};
}
// --- End: shard/syntax/SyntaxToken.h ---

// --- Begin: shard/syntax/SyntaxKind.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

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

		// Members
		FieldDeclaration,
		MethodDeclaration,
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
// --- End: shard/syntax/SyntaxKind.h ---

// --- Begin: shard/syntax/SyntaxNode.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

namespace shard
{
	class SHARD_API SyntaxNode
	{
	public:
		const SyntaxKind Kind;
		SyntaxNode *const Parent;

		inline SyntaxNode(const SyntaxKind kind, SyntaxNode *const parent)
			: Kind(kind), Parent(parent) { }

		inline SyntaxNode(const SyntaxNode& other) = delete;

		inline virtual ~SyntaxNode()
		{
			*const_cast<SyntaxNode**>(&Parent) = nullptr;
		}
	};
}
// --- End: shard/syntax/SyntaxNode.h ---

// --- Begin: shard/syntax/nodes/StatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

namespace shard
{
	class SHARD_API StatementSyntax : public SyntaxNode
	{
	public:
		SyntaxToken SemicolonToken;

		inline StatementSyntax(const SyntaxKind kind, SyntaxNode *const parent) : SyntaxNode(kind, parent)
		{
			/*
			if (kind < SyntaxKind::Statement || kind > SyntaxKind::ElseStatement)
				throw std::std::runtime_error("StatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline StatementSyntax(const StatementSyntax& other) = delete;

		inline virtual ~StatementSyntax()
		{

		}
	};

	class SHARD_API KeywordStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken KeywordToken;

		inline KeywordStatementSyntax(const SyntaxKind kind, SyntaxNode *const parent) : StatementSyntax(kind, parent)
		{
			/*
			if (kind < SyntaxKind::KeywordStatement || kind > SyntaxKind::ElseStatement)
				throw std::std::runtime_error("KeywordStatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline KeywordStatementSyntax(const KeywordStatementSyntax& other) = delete;

		inline virtual ~KeywordStatementSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/StatementSyntax.h ---

// --- Begin: shard/syntax/nodes/ExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

namespace shard
{
	class SHARD_API ExpressionSyntax : public SyntaxNode
	{
	public:
		inline ExpressionSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline ExpressionSyntax(const ExpressionSyntax& other) = delete;

		inline virtual ~ExpressionSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/ExpressionSyntax.h ---

// --- Begin: shard/syntax/SymbolAccesibility.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

namespace shard
{
	enum class SymbolAccesibility
	{
		Private,
		Public,
		Protected
	};
}
// --- End: shard/syntax/SymbolAccesibility.h ---

// --- Begin: shard/syntax/SyntaxSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SymbolAccesibility.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API SyntaxSymbol
	{
		inline static int counter = 0;

	public:
		const int TypeCode;
		const SyntaxKind Kind;
		const std::wstring Name;
		std::wstring FullName;
		SyntaxSymbol* Parent = nullptr;

		SymbolAccesibility Accesibility = SymbolAccesibility::Private;
		bool IsExtern = false;

		inline SyntaxSymbol(const std::wstring& name, const SyntaxKind kind)
			: TypeCode(counter++), Name(name), Kind(kind) { }

		inline SyntaxSymbol(const SyntaxSymbol& other) = delete;

		inline virtual ~SyntaxSymbol() = default;

		virtual void OnSymbolDeclared(SyntaxSymbol* symbol);

		bool IsType() const;
		bool IsMember() const;
	};
}
// --- End: shard/syntax/SyntaxSymbol.h ---

// --- Begin: shard/syntax/symbols/TypeSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

#include <string>
#include <vector>

// Forward declaration
namespace shard
{
    class TypeParameterSymbol;
    class MethodSymbol;
    class ConstructorSymbol;
    class FieldSymbol;
    class PropertySymbol;
    class IndexatorSymbol;
}

namespace shard
{
    enum class SHARD_API TypeLayoutingState
    {
        Unvisited,
        Visiting,
        Visited
    };

	class SHARD_API TypeSymbol : public SyntaxSymbol
	{
	public:
        TypeSymbol* BaseType = nullptr;
        std::vector<TypeSymbol*> Interfaces;
        std::vector<TypeParameterSymbol*> TypeParameters;

        std::vector<ConstructorSymbol*> Constructors;
        std::vector<MethodSymbol*> Methods;
        std::vector<IndexatorSymbol*> Indexators;
        std::vector<FieldSymbol*> Fields;
        std::vector<PropertySymbol*> Properties;

        TypeLayoutingState State = TypeLayoutingState::Unvisited;
        size_t MemoryBytesSize = 0;
        bool IsReferenceType = false;
        bool IsValueType = false;
        bool IsNullable = false;

        bool IsStatic = false;
        bool IsAbstract = false;
        bool IsSealed = false;

        inline TypeSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline TypeSymbol(const TypeSymbol& other) = delete;

        inline ~TypeSymbol() override
        {
#pragma warning (push)
#pragma warning (disable: 4150)
            for (MethodSymbol* methodSymbol : Methods)
                delete methodSymbol;

            for (FieldSymbol* fieldSymbol : Fields)
                delete fieldSymbol;

            for (PropertySymbol* propertySymbol : Properties)
                delete propertySymbol;

            for (IndexatorSymbol* indexatorSymbol : Indexators)
                delete indexatorSymbol;

            for (ConstructorSymbol* ctorSymbol : Constructors)
                delete ctorSymbol;

            for (TypeParameterSymbol* typeParamSymbol : TypeParameters)
                delete typeParamSymbol;
#pragma warning (pop)

            if (BaseType != nullptr)
                BaseType = nullptr;
        }

        inline size_t GetInlineSize() const
        {
            return IsReferenceType ? sizeof(void*) : MemoryBytesSize;
        }

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;

        virtual ConstructorSymbol* FindConstructor(std::vector<TypeSymbol*> parameterTypes);
        virtual MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes);
        virtual IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes);
        virtual FieldSymbol* FindField(std::wstring& name);
        virtual PropertySymbol* FindProperty(std::wstring& name);

        static bool Equals(const TypeSymbol* left, const TypeSymbol* right);
        bool IsPrimitive();

        static TypeSymbol* SubstituteType(TypeSymbol* type);
		static TypeSymbol* ReturnOf(FieldSymbol* field);
		static TypeSymbol* ReturnOf(MethodSymbol* method);
		static TypeSymbol* ReturnOf(PropertySymbol* property);
		static TypeSymbol* ReturnOf(IndexatorSymbol* indexator);
		static TypeSymbol* ReturnOf(ConstructorSymbol* constructor);
	};
}
// --- End: shard/syntax/symbols/TypeSymbol.h ---

// --- Begin: shard/syntax/nodes/TypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API TypeSyntax : public SyntaxNode
	{
	public:
		shard::TypeSymbol* Symbol = nullptr;

		inline TypeSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline TypeSyntax(const TypeSyntax& other) = delete;

		inline virtual ~TypeSyntax() { }

		inline virtual std::wstring ToString()
		{
			return L"<unknown>";
		}
	};
}
// --- End: shard/syntax/nodes/TypeSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/VariableStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API VariableStatementSyntax : public StatementSyntax
	{
	public:
		TypeSyntax* Type = nullptr;
		SyntaxToken IdentifierToken;
		SyntaxToken AssignToken;
		ExpressionSyntax* Expression = nullptr;

		inline VariableStatementSyntax(TypeSyntax* type, SyntaxToken name, SyntaxToken assignOp, ExpressionSyntax* expression, SyntaxNode *const parent)
			: StatementSyntax(SyntaxKind::VariableStatement, parent), Type(type), IdentifierToken(name), AssignToken(assignOp), Expression(expression) { }

		inline VariableStatementSyntax(const VariableStatementSyntax& other) = delete;

		inline virtual ~VariableStatementSyntax()
		{
			if (Type != nullptr)
			{
				Type->~TypeSyntax();
				delete Type;
			}

			if (Expression != nullptr)
			{
				Expression->~ExpressionSyntax();
				delete Expression;
			}
		}
	};
}
// --- End: shard/syntax/nodes/Statements/VariableStatementSyntax.h ---

// --- Begin: shard/syntax/symbols/FieldSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

#include <string>

namespace shard
{
    class SHARD_API FieldSymbol : public SyntaxSymbol
    {
    public:
        size_t MemoryBytesOffset = 0;
        TypeSymbol* ReturnType = nullptr;
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;
        bool IsStatic = false;

        inline FieldSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::FieldDeclaration) { }

        inline FieldSymbol(const FieldSymbol& other) = delete;

        inline virtual ~FieldSymbol()
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;
        }
    };
}
// --- End: shard/syntax/symbols/FieldSymbol.h ---

// --- Begin: shard/syntax/symbols/ParameterSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

// #include "shard/syntax/SymbolAccesibility.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

#include <string>
#include <cstdint>

namespace shard
{
	class SHARD_API ParameterSymbol : public SyntaxSymbol
	{
    public:
        TypeSymbol* Type = nullptr;
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;

        uint16_t SlotIndex = 0;
        bool IsOptional = false;

        inline ParameterSymbol(const std::wstring name) : SyntaxSymbol(name, SyntaxKind::Parameter)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const std::wstring name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::Parameter), Type(type)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const ParameterSymbol& other) = delete;

        inline virtual ~ParameterSymbol()
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;
        }
	};
}
// --- End: shard/syntax/symbols/ParameterSymbol.h ---

// --- Begin: shard/syntax/symbols/MethodSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/ParameterSymbol.h" (Merged)

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace shard
{
    class ObjectInstance;
    class VirtualMachine;
    class ArgumentsSpan;

    typedef SHARD_API shard::ObjectInstance* (*MethodSymbolDelegate)(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments);

    enum class SHARD_API MethodHandleType
    {
        None,
        Body,
        External,
        Lambda,
    };

    class SHARD_API MethodSymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        std::vector<ParameterSymbol*> Parameters;
        uint16_t EvalStackLocalsCount = 0;

        MethodHandleType HandleType = MethodHandleType::Body;
        std::vector<std::byte> ExecutableByteCode;
        MethodSymbolDelegate FunctionPointer = nullptr;

        bool IsStatic = false;

        inline MethodSymbol(const std::wstring& name)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, MethodSymbolDelegate delegate)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::External) { }

        inline MethodSymbol(const MethodSymbol& other) = delete;

        inline virtual ~MethodSymbol() override
        {
            for (ParameterSymbol* parameter : Parameters)
                delete parameter;

            if (FunctionPointer != nullptr)
                FunctionPointer = nullptr;

            for (ParameterSymbol* param : Parameters)
                delete param;
        }
    };
}
// --- End: shard/syntax/symbols/MethodSymbol.h ---

// --- Begin: shard/syntax/symbols/AccessorSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <string>

namespace shard
{
    class SHARD_API AccessorSymbol : public MethodSymbol
    {
    public:
        inline AccessorSymbol(std::wstring name) : MethodSymbol(name)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, SyntaxKind::AccessorDeclaration)
        {
            FunctionPointer = delegate;
            HandleType = MethodHandleType::External;
            Accesibility = SymbolAccesibility::Public;
        }

        inline AccessorSymbol(const AccessorSymbol& other) = delete;

        inline virtual ~AccessorSymbol()
        {

        }
    };
}
// --- End: shard/syntax/symbols/AccessorSymbol.h ---

// --- Begin: shard/syntax/symbols/PropertySymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/AccessorSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

#include <string>

namespace shard
{
    class SHARD_API PropertySymbol : public SyntaxSymbol
    {
    public:
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;
        TypeSymbol* ReturnType = nullptr;
        AccessorSymbol* Getter = nullptr;
        AccessorSymbol* Setter = nullptr;
        FieldSymbol* BackingField = nullptr;
        bool IsStatic = false;

        inline PropertySymbol(const std::wstring& name)
            : SyntaxSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline PropertySymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline PropertySymbol(const PropertySymbol& other) = delete;

        inline virtual ~PropertySymbol() override
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;

            if (Getter != nullptr)
                delete Getter;

            if (Setter != nullptr)
                delete Setter;

            if (BackingField != nullptr)
                delete BackingField;
        }

        PropertySymbol* GenerateBackingField();
    };
}

// --- End: shard/syntax/symbols/PropertySymbol.h ---

// --- Begin: shard/syntax/symbols/IndexatorSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/PropertySymbol.h" (Merged)
// #include "shard/syntax/symbols/ParameterSymbol.h" (Merged)

#include <vector>

namespace shard
{
    class PropertySymbol;

    class SHARD_API IndexatorSymbol : public PropertySymbol
    {
    public:
        std::vector<ParameterSymbol*> Parameters;

        inline IndexatorSymbol(const std::wstring& name)
            : PropertySymbol(name, SyntaxKind::IndexatorDeclaration) { }

        inline IndexatorSymbol(const IndexatorSymbol& other) = delete;

        inline ~IndexatorSymbol() override
        {
            for (ParameterSymbol* param : Parameters)
                delete param;
        }
    };
}

// --- End: shard/syntax/symbols/IndexatorSymbol.h ---

// --- Begin: shard/syntax/nodes/ArgumentsListSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API ArgumentSyntax : public SyntaxNode
	{
	public:
		const ExpressionSyntax* Expression;
		const bool IsByReference;

		inline ArgumentSyntax(const ExpressionSyntax* expression, SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::Argument, parent), Expression(expression), IsByReference(false) { }

		inline ArgumentSyntax(const ArgumentSyntax& other) = delete;

		inline virtual ~ArgumentSyntax()
		{
			Expression->~ExpressionSyntax();
			delete Expression;
		}
	};

	class SHARD_API ArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline ArgumentsListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ArgumentsList, parent) { }

		inline ArgumentsListSyntax(const ArgumentsListSyntax& other) = delete;

		inline virtual ~ArgumentsListSyntax()
		{
			for (const ArgumentSyntax* argument : Arguments)
			{
				argument->~ArgumentSyntax();
				delete argument;
			}
		}
	};

	class SHARD_API IndexatorListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline IndexatorListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::IndexatorList, parent) { }

		inline IndexatorListSyntax(const IndexatorListSyntax& other) = delete;

		inline virtual ~IndexatorListSyntax()
		{
			for (const ArgumentSyntax* argument : Arguments)
			{
				argument->~ArgumentSyntax();
				delete argument;
			}
		}
	};
}
// --- End: shard/syntax/nodes/ArgumentsListSyntax.h ---

// --- Begin: shard/syntax/symbols/DelegateTypeSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/ParameterSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)

#include <new>
#include <vector>

namespace shard
{
	class SHARD_API DelegateTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* ReturnType = nullptr;
		std::vector<ParameterSymbol*> Parameters;
		MethodSymbol* AnonymousSymbol = nullptr;

		inline DelegateTypeSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::DelegateType)
		{
			IsReferenceType = true;
		}

		inline DelegateTypeSymbol(const DelegateTypeSymbol& other) = delete;

		inline virtual ~DelegateTypeSymbol()
		{
			if (AnonymousSymbol != nullptr)
				delete AnonymousSymbol;

			for (ParameterSymbol* parameter : Parameters)
				delete parameter;
		}
	};
}
// --- End: shard/syntax/symbols/DelegateTypeSymbol.h ---

// --- Begin: shard/syntax/symbols/VariableSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/SymbolAccesibility.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API VariableSymbol : public SyntaxSymbol
	{
	public:
		const TypeSymbol* Type = nullptr;
        shard::ExpressionSyntax* Declaration = nullptr;

		uint16_t SlotIndex = 0;
        bool IsConst = false;

		inline VariableSymbol(std::wstring name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::VariableStatement), Type(type)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline VariableSymbol(const VariableSymbol& other) = delete;

		inline virtual ~VariableSymbol() override
		{
			if (Declaration == nullptr)
			{
				delete Declaration;
				Declaration = nullptr;
			}
		}
	};
}
// --- End: shard/syntax/symbols/VariableSymbol.h ---

// --- Begin: shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)

// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/PropertySymbol.h" (Merged)
// #include "shard/syntax/symbols/IndexatorSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/VariableSymbol.h" (Merged)
// #include "shard/syntax/symbols/DelegateTypeSymbol.h" (Merged)

namespace shard
{
	class SHARD_API LinkedExpressionNode : public ExpressionSyntax
	{
	public:
		SyntaxToken DelimeterToken;
		ExpressionSyntax *const PreviousExpression = nullptr;
		bool IsStaticContext = true;

		inline LinkedExpressionNode(const SyntaxKind kind, ExpressionSyntax *const previous, SyntaxNode *const parent)
			: ExpressionSyntax(kind, parent), PreviousExpression(previous) { }

		inline LinkedExpressionNode(const LinkedExpressionNode&) = delete;

		inline virtual ~LinkedExpressionNode()
		{
			if (PreviousExpression != nullptr)
				delete PreviousExpression;
		}
	};

	class SHARD_API MemberAccessExpressionSyntax : public LinkedExpressionNode
	{
	public:
		shard::ParameterSymbol* ToParameter = nullptr;
		shard::VariableSymbol* ToVariable = nullptr;
		shard::FieldSymbol* ToField = nullptr;
		shard::PropertySymbol* ToProperty = nullptr;
		shard::DelegateTypeSymbol* ToDelegate = nullptr;

		const SyntaxToken IdentifierToken;

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, previous, parent), IdentifierToken(identifier) { }

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode* const parent, const SyntaxKind kind)
			: LinkedExpressionNode(kind, previous, parent), IdentifierToken(identifier) {
		}

		inline MemberAccessExpressionSyntax(const MemberAccessExpressionSyntax&) = delete;

		inline virtual ~MemberAccessExpressionSyntax()
		{
			ToParameter = nullptr;
			ToVariable = nullptr;
			ToField = nullptr;
			ToProperty = nullptr;
			ToDelegate = nullptr;
		}
	};

	class SHARD_API InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::MethodSymbol* Symbol = nullptr;

		inline InvokationExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, previous, parent), IdentifierToken(identifier) { }

		inline InvokationExpressionSyntax(const InvokationExpressionSyntax&) = delete;

		inline virtual ~InvokationExpressionSyntax()
		{
			delete ArgumentsList;
		}
	};

	class SHARD_API IndexatorExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		IndexatorListSyntax* IndexatorList = nullptr;
		shard::IndexatorSymbol* IndexatorSymbol = nullptr;

		inline IndexatorExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: MemberAccessExpressionSyntax(identifier, previous, parent, SyntaxKind::IndexatorExpression) { }

		inline IndexatorExpressionSyntax(const IndexatorExpressionSyntax&) = delete;

		inline virtual ~IndexatorExpressionSyntax()
		{
			delete IndexatorList;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/BreakStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

namespace shard
{
	class SHARD_API BreakStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline BreakStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::BreakStatement, parent) {}
		inline BreakStatementSyntax(const BreakStatementSyntax& other) = delete;
		inline virtual ~BreakStatementSyntax() {}
	};
}
// --- End: shard/syntax/nodes/Statements/BreakStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API IdentifierNameTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken Identifier;

		inline IdentifierNameTypeSyntax(SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

		inline IdentifierNameTypeSyntax(const IdentifierNameTypeSyntax& other) = delete;

		inline virtual ~IdentifierNameTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h ---

// --- Begin: shard/syntax/symbols/NamespaceSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class NamespaceNode;
}

namespace shard
{
	class SHARD_API NamespaceSymbol : public SyntaxSymbol
	{
	public:
		std::vector<SyntaxSymbol*> Members;
		shard::NamespaceNode* Node = nullptr;

		inline NamespaceSymbol(std::wstring name)
			: SyntaxSymbol(name, SyntaxKind::NamespaceDeclaration) { }

		inline NamespaceSymbol(const NamespaceSymbol& other) = delete;

		void OnSymbolDeclared(SyntaxSymbol* symbol) override;

		inline virtual ~NamespaceSymbol()
		{
			for (SyntaxSymbol* member : Members)
				delete member;
		}
	};
}
// --- End: shard/syntax/symbols/NamespaceSymbol.h ---

// --- Begin: shard/parsing/semantic/NamespaceTree.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/NamespaceSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <unordered_map>
#include <vector>
#include <string>

namespace shard
{
	class NamespaceSymbol;
}

namespace shard
{
	class SHARD_API NamespaceNode
	{
	public:
		std::vector<shard::NamespaceSymbol*> Owners;
		std::vector<shard::TypeSymbol*> Types;
		std::unordered_map<std::wstring, NamespaceNode*> Nodes;

		inline NamespaceNode()
		{

		}

		inline ~NamespaceNode()
		{
			for (const auto& node : Nodes)
				delete node.second;
		}

		NamespaceNode* Lookup(std::wstring name);
		NamespaceNode* LookupOrCreate(std::wstring name, shard::NamespaceSymbol* current);
	};

	class SHARD_API NamespaceTree
	{
	public:
		NamespaceNode* Root;

		inline NamespaceTree()
			: Root(new NamespaceNode()) { }

		inline ~NamespaceTree()
		{
			delete Root;
		}
	};
}
// --- End: shard/parsing/semantic/NamespaceTree.h ---

// --- Begin: shard/syntax/nodes/Directives/UsingDirectiveSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

#include <vector>
#include <string>

namespace shard
{
	class SHARD_API UsingDirectiveSyntax : public SyntaxNode
	{
	public:
		SyntaxToken UsingKeywordToken;
		SyntaxToken SemicolonToken;
		std::vector<SyntaxToken> TokensList;
		shard::NamespaceNode* Namespace = nullptr;

		inline UsingDirectiveSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::UsingDirective, parent) { }

		inline UsingDirectiveSyntax(const UsingDirectiveSyntax&) = delete;

		inline virtual ~UsingDirectiveSyntax()
		{

		}

		std::wstring ToString();
	};
}
// --- End: shard/syntax/nodes/Directives/UsingDirectiveSyntax.h ---

// --- Begin: shard/syntax/nodes/TypeParametersListSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API TypeParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<SyntaxToken> Types;

		inline TypeParametersListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) { }

		inline TypeParametersListSyntax(const TypeParametersListSyntax& other) = delete;

		inline virtual ~TypeParametersListSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/TypeParametersListSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/nodes/TypeParametersListSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
	public:
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken DeclareToken;
		SyntaxToken IdentifierToken;
		TypeParametersListSyntax* TypeParameters = nullptr;

		inline MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline MemberDeclarationSyntax(const MemberDeclarationSyntax& other) = delete;

		inline virtual ~MemberDeclarationSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/CompilationUnitSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/Directives/UsingDirectiveSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API CompilationUnitSyntax : public SyntaxNode
	{
	public:
		std::vector<UsingDirectiveSyntax*> Usings;
		std::vector<MemberDeclarationSyntax*> Members;

		inline CompilationUnitSyntax()
			: SyntaxNode(SyntaxKind::CompilationUnit, nullptr) { }

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other) = delete;

		inline virtual ~CompilationUnitSyntax()
		{
			for (const UsingDirectiveSyntax* usingDirective : Usings)
				delete usingDirective;

			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
// --- End: shard/syntax/nodes/CompilationUnitSyntax.h ---

// --- Begin: shard/parsing/SyntaxTree.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
#include <vector>

namespace shard
{
	class SHARD_API SyntaxTree
	{
	public:
		std::vector<shard::CompilationUnitSyntax*> CompilationUnits;

		inline SyntaxTree() { }

		inline virtual ~SyntaxTree()
		{
			for (const auto unit : CompilationUnits)
				delete unit;
		}
	};
}
// --- End: shard/parsing/SyntaxTree.h ---

// --- Begin: shard/parsing/semantic/SemanticScope.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

#include <string>
#include <unordered_map>

namespace shard
{
	class SHARD_API SemanticScope
	{
	public:
		std::unordered_map<std::wstring, shard::SyntaxSymbol*> _symbols;

		SemanticScope *const Parent;
		shard::SyntaxSymbol *const Owner;
		NamespaceNode* Namespace = nullptr;

		bool ReturnFound = false;
		bool ReturnsAnything = false;

		inline SemanticScope(shard::SyntaxSymbol *const owner, SemanticScope *const parent)
			: Owner(owner), Parent(parent) { }

		shard::SyntaxSymbol *const Lookup(const std::wstring& name);
		void DeclareSymbol(shard::SyntaxSymbol *const symbol);
		void RemoveSymbol(shard::SyntaxSymbol *const symbol);
	};
}
// --- End: shard/parsing/semantic/SemanticScope.h ---

// --- Begin: shard/parsing/semantic/SymbolTable.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/NamespaceSymbol.h" (Merged)

// #include "shard/parsing/semantic/SemanticScope.h" (Merged)

#include <unordered_map>
#include <vector>
#include <string>
#include <ranges>

namespace shard
{
    class SHARD_API SymbolTable
    {
        inline static const std::wstring GlobalTypeName = L"__GLOBAL__";

        std::unordered_map<shard::SyntaxNode*, shard::SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<shard::SyntaxSymbol*, shard::SyntaxNode*> symbolToNodeMap;
        std::vector<shard::NamespaceSymbol*> namespacesList;
        std::vector<shard::TypeSymbol*> typesList;

    public:
        struct Global
        {
            inline static SHARD_API shard::TypeSymbol *const Type = new shard::TypeSymbol(GlobalTypeName, shard::SyntaxKind::CompilationUnit);
            inline static SHARD_API shard::SemanticScope *const Scope = new SemanticScope(Type, nullptr);
        };

        struct Primitives
        {
            inline static SHARD_API shard::TypeSymbol* Void;
            inline static SHARD_API shard::TypeSymbol* Null;
            inline static SHARD_API shard::TypeSymbol* Any;

            inline static SHARD_API shard::TypeSymbol* Boolean;
            inline static SHARD_API shard::TypeSymbol* Integer;
            inline static SHARD_API shard::TypeSymbol* Double;
            inline static SHARD_API shard::TypeSymbol* Char;
            inline static SHARD_API shard::TypeSymbol* String;
            inline static SHARD_API shard::TypeSymbol* Array;
        };

        inline SymbolTable()
        {

        }

        ~SymbolTable();

        void ClearSymbols();
        void BindSymbol(shard::SyntaxNode *const node, shard::SyntaxSymbol *const symbol);
        shard::SyntaxSymbol *const LookupSymbol(shard::SyntaxNode *const node);
        shard::SyntaxNode *const GetSyntaxNode(shard::SyntaxSymbol *const symbol);
        const std::vector<shard::NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<shard::TypeSymbol*> GetTypeSymbols();
    };
}
// --- End: shard/parsing/semantic/SymbolTable.h ---

// --- Begin: shard/parsing/semantic/SymbolInfo.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

namespace shard
{
    struct SHARD_API SymbolInfo
    {
    public:
        shard::SyntaxSymbol* Symbol;

        inline SymbolInfo(shard::SyntaxSymbol* symbol)
            : Symbol(symbol) { }
    };
}
// --- End: shard/parsing/semantic/SymbolInfo.h ---

// --- Begin: shard/parsing/semantic/TypeInfo.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

namespace shard
{
    struct SHARD_API TypeInfo
    {
    public:
        shard::TypeSymbol* Type;

        inline TypeInfo(shard::TypeSymbol* type)
            : Type(type) { }
    };
}
// --- End: shard/parsing/semantic/TypeInfo.h ---

// --- Begin: shard/parsing/semantic/SemanticModel.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/parsing/SyntaxTree.h" (Merged)

// #include "shard/parsing/semantic/SymbolTable.h" (Merged)
// #include "shard/parsing/semantic/TypeInfo.h" (Merged)
// #include "shard/parsing/semantic/SymbolInfo.h" (Merged)
// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

namespace shard
{
	class SHARD_API SemanticModel
	{
	public:
		shard::SyntaxTree& Tree;
		shard::SymbolTable* Table;
		shard::NamespaceTree* Namespaces;

		SemanticModel(shard::SyntaxTree& tree);
		~SemanticModel();

		shard::SymbolInfo GetSymbolInfo(shard::SyntaxNode* node);
		shard::TypeInfo GetTypeInfo(shard::ExpressionSyntax* expression);
		//Conversion ClassifyConversion(shard::ExpressionSyntax* expression, shard::TypeSymbol destination);
		//DataFlowAnalysis AnalyzeDataFlow(shard::SyntaxNode* firstNode, shard::SyntaxNode* lastNode);
	};
}
// --- End: shard/parsing/semantic/SemanticModel.h ---

// --- Begin: shard/syntax/nodes/BodyDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

namespace shard
{
	class SHARD_API BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;

		inline BodyDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: MemberDeclarationSyntax(kind, parent) { }

		inline BodyDeclarationSyntax(const BodyDeclarationSyntax& other) = delete;

		inline virtual ~BodyDeclarationSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/BodyDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/StatementsBlockSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/BodyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<StatementSyntax*> Statements;

		inline StatementsBlockSyntax(SyntaxNode *const parent)
			: BodyDeclarationSyntax(SyntaxKind::StatementsBlock, parent) { }

		inline StatementsBlockSyntax(const StatementsBlockSyntax& other) = delete;

		inline virtual ~StatementsBlockSyntax()
		{
			for (const StatementSyntax* statement : Statements)
				delete statement;
		}
	};
}
// --- End: shard/syntax/nodes/StatementsBlockSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API AccessorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken KeywordToken;
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken SemicolonToken;
		StatementsBlockSyntax* Body = nullptr;

		inline AccessorDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent) { }

		inline AccessorDeclarationSyntax(SyntaxNode *const parent, StatementsBlockSyntax* body)
			: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent), Body(body) { }

		inline AccessorDeclarationSyntax(const AccessorDeclarationSyntax&) = delete;

		inline ~AccessorDeclarationSyntax() override
		{

		}
	};
}


// --- End: shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/ParametersListSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API ParameterSyntax : public SyntaxNode
	{
	public:
		const TypeSyntax* Type;
		const SyntaxToken Identifier;
		shard::TypeSymbol* Symbol = nullptr;

		inline ParameterSyntax(const TypeSyntax* type, const SyntaxToken identifier, SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::Parameter, parent), Type(type), Identifier(identifier) { }

		inline ParameterSyntax(const ParameterSyntax& other) = delete;

		inline virtual ~ParameterSyntax()
		{

		}
	};

	class SHARD_API ParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<ParameterSyntax*> Parameters;

		inline ParametersListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {}

		inline ParametersListSyntax(const ParametersListSyntax& other) = delete;

		inline virtual ~ParametersListSyntax()
		{
			for (const ParameterSyntax* parameter : Parameters)
				delete parameter;
		}
	};
}
// --- End: shard/syntax/nodes/ParametersListSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/DelegateTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API DelegateTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken DelegateToken;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateTypeSyntax(SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::DelegateType, parent) { }

		inline DelegateTypeSyntax(const DelegateTypeSyntax& other) = delete;

		inline virtual ~DelegateTypeSyntax()
		{
			if (Params != nullptr)
				delete Params;
		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/DelegateTypeSyntax.h ---

// --- Begin: shard/syntax/nodes/TypeArgumentsListSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API TypeArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<TypeSyntax*> Types;

		inline TypeArgumentsListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {
		}

		inline TypeArgumentsListSyntax(const TypeArgumentsListSyntax& other) = delete;

		inline virtual ~TypeArgumentsListSyntax()
		{
			for (const TypeSyntax* parameter : Types)
				delete parameter;
		}
	};
}
// --- End: shard/syntax/nodes/TypeArgumentsListSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/GenericTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeArgumentsListSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API GenericTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		TypeArgumentsListSyntax* Arguments = nullptr;

		inline GenericTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::GenericType, parent), UnderlayingType(underlaying) { }

		inline GenericTypeSyntax(const GenericTypeSyntax& other) = delete;

		inline virtual ~GenericTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;

			if (Arguments != nullptr)
				delete Arguments;
		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/GenericTypeSyntax.h ---

// --- Begin: shard/parsing/analysis/DiagnosticSeverity.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

#include <string>

namespace shard
{
	enum class DiagnosticSeverity
	{
		Info,
		Warning,
		Error
	};
}

SHARD_API std::wstring severity_to_wstring(const shard::DiagnosticSeverity& severity);
// --- End: shard/parsing/analysis/DiagnosticSeverity.h ---

// --- Begin: shard/parsing/analysis/Diagnostic.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/analysis/TextLocation.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticSeverity.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API Diagnostic
	{
	public:
		const DiagnosticSeverity Severity;
		const std::wstring Description;
		const shard::SyntaxToken Token;
		const TextLocation Location;

		Diagnostic(shard::SyntaxToken token, DiagnosticSeverity severity, std::wstring description);
		Diagnostic(const Diagnostic& other);

		inline Diagnostic& operator=(const Diagnostic& other)
		{
			if (this != &other)
			{
				this->~Diagnostic();
				new (this) Diagnostic(other);
			}

			return *this;
		}
	};
}
// --- End: shard/parsing/analysis/Diagnostic.h ---

// --- Begin: shard/parsing/analysis/DiagnosticsContext.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/parsing/analysis/Diagnostic.h" (Merged)

#include <string>
#include <vector>
#include <ostream>

namespace shard
{
	class SHARD_API DiagnosticsContext
	{
	public:
		bool AnyError = false;
		std::vector<Diagnostic> Diagnostics;

		inline DiagnosticsContext() : Diagnostics() {}

		void ReportError(shard::SyntaxToken token, std::wstring message);
		void ReportWarning(shard::SyntaxToken token, std::wstring message);
		void ReportInfo(shard::SyntaxToken token, std::wstring message);
		void WriteDiagnostics(std::wostream& out);
		void Reset();
	};
}
// --- End: shard/parsing/analysis/DiagnosticsContext.h ---

// --- Begin: shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API LiteralExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken LiteralToken;

		inline LiteralExpressionSyntax(const SyntaxToken literal, SyntaxNode *const parent) : ExpressionSyntax(SyntaxKind::LiteralExpression, parent), LiteralToken(literal) { }
		inline LiteralExpressionSyntax(const LiteralExpressionSyntax&) = delete;
		inline virtual ~LiteralExpressionSyntax() { }
	};
}
// --- End: shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/symbols/DelegateTypeSymbol.h" (Merged)

namespace shard
{
	class SHARD_API LambdaExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken LambdaToken;
		SyntaxToken LambdaOperatorToken;
		ParametersListSyntax* Params = nullptr;
		StatementsBlockSyntax* Body = nullptr;
		shard::DelegateTypeSymbol* Symbol = nullptr;

		inline LambdaExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

		inline LambdaExpressionSyntax(const LambdaExpressionSyntax&) = delete;

		inline virtual ~LambdaExpressionSyntax()
		{
			if (Params != nullptr)
				delete Params;

			if (Body != nullptr)
				delete Body;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/Loops/WhileStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API WhileStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		ExpressionSyntax* ConditionExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline WhileStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::WhileStatement, parent) { }

		inline WhileStatementSyntax(const WhileStatementSyntax&) = delete;

		inline virtual ~WhileStatementSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
// --- End: shard/syntax/nodes/Loops/WhileStatementSyntax.h ---

// --- Begin: shard/parsing/MemberDeclarationInfo.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/nodes/TypeParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <vector>

namespace shard
{
	struct SHARD_API MemberDeclarationInfo
	{
		std::vector<shard::SyntaxToken> Modifiers;
		shard::SyntaxToken DeclareType;
		shard::SyntaxToken Identifier;
		shard::TypeSyntax* ReturnType = nullptr;
		shard::TypeParametersListSyntax* Generics = nullptr;
	};
}
// --- End: shard/parsing/MemberDeclarationInfo.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API MethodDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;
		StatementsBlockSyntax* Body = nullptr;

		inline MethodDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::MethodDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline MethodDeclarationSyntax(const MethodDeclarationSyntax&) = delete;

		inline ~MethodDeclarationSyntax() override
		{
			if (ReturnType != nullptr)
				delete ReturnType;

			if (Params != nullptr)
				delete Params;

			if (Body != nullptr)
				delete Body;
		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/ConditionalClauseSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

namespace shard
{
	class SHARD_API ConditionalClauseBaseSyntax : public KeywordStatementSyntax
	{
	public:
		StatementsBlockSyntax* StatementsBlock = nullptr;
		ConditionalClauseBaseSyntax* NextStatement = nullptr;

		inline ConditionalClauseBaseSyntax(const SyntaxKind kind, SyntaxNode *const parent) : KeywordStatementSyntax(kind, parent) { }
		inline ConditionalClauseBaseSyntax(const ConditionalClauseBaseSyntax& other) = delete;

		inline virtual ~ConditionalClauseBaseSyntax()
		{
			if (StatementsBlock != nullptr)
				delete StatementsBlock;

			if (NextStatement != nullptr)
				delete NextStatement;
		}
	};

	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		StatementSyntax* ConditionExpression = nullptr;

		inline ConditionalClauseSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: ConditionalClauseBaseSyntax(kind, parent) { }

		inline ConditionalClauseSyntax(const ConditionalClauseSyntax& other) = delete;

		inline virtual ~ConditionalClauseSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;
		}
	};

	class SHARD_API IfStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline IfStatementSyntax(SyntaxNode *const parent) : ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }
		inline IfStatementSyntax(const IfStatementSyntax& other) = delete;
		inline virtual ~IfStatementSyntax() { }
	};

	class SHARD_API UnlessStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline UnlessStatementSyntax(SyntaxNode *const parent) : ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }
		inline UnlessStatementSyntax(const UnlessStatementSyntax& other) = delete;
		inline virtual ~UnlessStatementSyntax() { }
	};

	class SHARD_API ElseStatementSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		inline ElseStatementSyntax(SyntaxNode *const parent) : ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }
		inline ElseStatementSyntax(const ElseStatementSyntax& other) = delete;
		inline virtual ~ElseStatementSyntax() { }
	};
}
// --- End: shard/syntax/nodes/Statements/ConditionalClauseSyntax.h ---

// --- Begin: shard/syntax/nodes/Loops/UntilStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API UntilStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		ExpressionSyntax* ConditionExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline UntilStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::UntilStatement, parent) { }

		inline UntilStatementSyntax(const UntilStatementSyntax&) = delete;

		inline virtual ~UntilStatementSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
// --- End: shard/syntax/nodes/Loops/UntilStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)

#include <vector>

namespace shard
{
    class SHARD_API IndexatorDeclarationSyntax : public MemberDeclarationSyntax
    {
    public:
        SyntaxToken IndexKeyword;
        SyntaxToken OpenBraceToken;
        SyntaxToken CloseBraceToken;

        TypeSyntax* ReturnType = nullptr;
        ParametersListSyntax* Parameters = nullptr;

        AccessorDeclarationSyntax* Getter = nullptr;
        AccessorDeclarationSyntax* Setter = nullptr;

        inline IndexatorDeclarationSyntax(SyntaxNode *const parent)
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent) { }

        inline IndexatorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent)
            : MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent)
        {
            Modifiers = info.Modifiers;
            IdentifierToken = info.Identifier;
            ReturnType = info.ReturnType;
            TypeParameters = info.Generics;
        }

        inline virtual ~IndexatorDeclarationSyntax()
        {
            if (Getter != nullptr)
            {
                delete Getter;
                Getter = nullptr;
            }

            if (Setter != nullptr)
            {
                delete Setter;
                Setter = nullptr;
            }

            if (ReturnType != nullptr)
            {
                delete ReturnType;
                ReturnType = nullptr;
            }

            if (Parameters != nullptr)
            {
                delete Parameters;
                Parameters = nullptr;
            }
        }
    };
}
// --- End: shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/TypeDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/BodyDeclarationSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API TypeDeclarationSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<MemberDeclarationSyntax*> Members;

		inline TypeDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: BodyDeclarationSyntax(kind, parent) { }

		inline TypeDeclarationSyntax(const TypeDeclarationSyntax& other) = delete;

		inline virtual ~TypeDeclarationSyntax()
		{
			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
// --- End: shard/syntax/nodes/TypeDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/TypeDeclarationSyntax.h" (Merged)
// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API StructDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline StructDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent) { }

		inline StructDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::StructDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline StructDeclarationSyntax(const StructDeclarationSyntax& other) = delete;

		inline virtual ~StructDeclarationSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)
#include <vector>

namespace shard
{
	class SHARD_API ConstructorDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken Semicolon;
		StatementsBlockSyntax* Body = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline ConstructorDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::ConstructorDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline ConstructorDeclarationSyntax(const ConstructorDeclarationSyntax&) = delete;

		inline ~ConstructorDeclarationSyntax() override
		{
			if (Body != nullptr)
				delete Body;

			if (Params != nullptr)
				delete Params;
		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h ---

// --- Begin: shard/syntax/symbols/ConstructorSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <string>

namespace shard
{
    class SHARD_API ConstructorSymbol : public MethodSymbol
    {
    public:
        inline ConstructorSymbol(std::wstring name)
            : MethodSymbol(name, SyntaxKind::ConstructorDeclaration) { }

        inline ConstructorSymbol(std::wstring name, MethodSymbolDelegate delegate) : MethodSymbol(name, SyntaxKind::ConstructorDeclaration)
        {
            FunctionPointer = delegate;
            HandleType = MethodHandleType::External;
        }

        inline ConstructorSymbol(const ConstructorSymbol& other) = delete;

        inline virtual ~ConstructorSymbol()
        {

        }
    };
}
// --- End: shard/syntax/symbols/ConstructorSymbol.h ---

// --- Begin: shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)

namespace shard
{
	class SHARD_API ObjectExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken NewToken;
		SyntaxToken IdentifierToken;

		TypeSyntax* Type = nullptr;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::TypeSymbol* TypeSymbol = nullptr;
		shard::ConstructorSymbol* CtorSymbol = nullptr;

		inline ObjectExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

		inline ObjectExpressionSyntax(const ObjectExpressionSyntax&) = delete;

		inline virtual ~ObjectExpressionSyntax()
		{
			if (Type != nullptr)
				delete Type;

			if (ArgumentsList != nullptr)
				delete ArgumentsList;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h ---

// --- Begin: shard/syntax/symbols/ArrayTypeSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/PropertySymbol.h" (Merged)
// #include "shard/syntax/symbols/IndexatorSymbol.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/parsing/semantic/SymbolTable.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ArrayTypeSymbol : public TypeSymbol
	{
	public:
		TypeSymbol* UnderlayingType = nullptr;
		size_t Size = 0;
		int Rank = 0;

		inline ArrayTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(L"Array", SyntaxKind::ArrayType), UnderlayingType(underlayingType)
		{
			MemoryBytesSize = shard::SymbolTable::Primitives::Array->MemoryBytesSize;
			IsReferenceType = true;
		}

		inline ArrayTypeSymbol(const ArrayTypeSymbol& other) = delete;

		inline virtual ~ArrayTypeSymbol()
		{

		}

		ConstructorSymbol* FindConstructor(std::vector<TypeSymbol*> parameterTypes) override;
		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}
// --- End: shard/syntax/symbols/ArrayTypeSymbol.h ---

// --- Begin: shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/symbols/ArrayTypeSymbol.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API CollectionExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ExpressionSyntax*> ValuesExpressions;
		shard::ArrayTypeSymbol* Symbol = nullptr;

		inline CollectionExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline CollectionExpressionSyntax(const CollectionExpressionSyntax&) = delete;

		inline virtual ~CollectionExpressionSyntax()
		{
			for (ExpressionSyntax* expression : ValuesExpressions)
				delete expression;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/ContinueStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

namespace shard
{
	class SHARD_API ContinueStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline ContinueStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ContinueStatement, parent) {}
		inline ContinueStatementSyntax(const ContinueStatementSyntax& other) = delete;

		inline virtual ~ContinueStatementSyntax() {}
	};
}
// --- End: shard/syntax/nodes/Statements/ContinueStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/ArrayTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API ArrayTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		int Rank = 1;

		inline ArrayTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::ArrayType, parent), UnderlayingType(underlaying) { }

		inline ArrayTypeSyntax(const ArrayTypeSyntax& other) = delete;

		inline virtual ~ArrayTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;
		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/ArrayTypeSyntax.h ---

// --- Begin: shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

namespace shard
{
	class SHARD_API UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Expression = nullptr;
		const bool IsRightDetermined;

		inline UnaryExpressionSyntax(const SyntaxToken operatorToken, const bool isRightDetermined, SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::UnaryExpression, parent), OperatorToken(operatorToken), IsRightDetermined(isRightDetermined) { }

		inline UnaryExpressionSyntax(const UnaryExpressionSyntax&) = delete;

		inline virtual ~UnaryExpressionSyntax()
		{
			delete Expression;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

// #include "shard/syntax/nodes/TypeDeclarationSyntax.h" (Merged)
// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API NamespaceDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		std::vector<SyntaxToken> IdentifierTokens;

		inline NamespaceDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent) { }

		inline NamespaceDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::NamespaceDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline NamespaceDeclarationSyntax(const NamespaceDeclarationSyntax& other) = delete;

		inline virtual ~NamespaceDeclarationSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/ExpressionStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

namespace shard
{
	class SHARD_API ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ExpressionStatementSyntax(SyntaxNode *const parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }
		inline ExpressionStatementSyntax(ExpressionSyntax* expression, SyntaxNode *const parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent), Expression(expression) { }
		inline ExpressionStatementSyntax(const ExpressionStatementSyntax& other) = delete;

		inline virtual ~ExpressionStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
// --- End: shard/syntax/nodes/Statements/ExpressionStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/ReturnStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

namespace shard
{
	class SHARD_API ReturnStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ReturnStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ReturnStatement, parent) { }
		inline ReturnStatementSyntax(const ReturnStatementSyntax& other) = delete;

		inline virtual ~ReturnStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
// --- End: shard/syntax/nodes/Statements/ReturnStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API PropertyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;

		TypeSyntax* ReturnType = nullptr;

		AccessorDeclarationSyntax* Getter = nullptr;
		AccessorDeclarationSyntax* Setter = nullptr;

		ExpressionSyntax* InitializerExpression = nullptr;

		inline PropertyDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

		inline PropertyDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline PropertyDeclarationSyntax(const PropertyDeclarationSyntax& other) = delete;

		inline virtual ~PropertyDeclarationSyntax()
		{
			if (Getter != nullptr)
			{
				delete Getter;
				Getter = nullptr;
			}

			if (Setter != nullptr)
			{
				delete Setter;
				Setter = nullptr;
			}

			if (InitializerExpression != nullptr)
			{
				delete InitializerExpression;
				InitializerExpression = nullptr;
			}

			if (ReturnType != nullptr)
			{
				delete ReturnType;
				ReturnType = nullptr;
			}
		}
	};
}

// --- End: shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API FieldDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken ReturnTypeToken;
		SyntaxToken SemicolonToken;
		SyntaxToken InitializerAssignToken;

		ExpressionSyntax* InitializerExpression = nullptr;
		TypeSyntax* ReturnType = nullptr;

		inline FieldDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent) { }

		inline FieldDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			ReturnType = info.ReturnType;
			TypeParameters = info.Generics;
		}

		inline FieldDeclarationSyntax(const FieldDeclarationSyntax& other) = delete;

		inline virtual ~FieldDeclarationSyntax()
		{
			if (InitializerExpression != nullptr)
				delete InitializerExpression;

			if (ReturnType != nullptr)
				delete ReturnType;
		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/Statements/ThrowStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)

namespace shard
{
	class SHARD_API ThrowStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ThrowStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ThrowStatement, parent) {}
		inline ThrowStatementSyntax(const ThrowStatementSyntax& other) = delete;

		inline virtual ~ThrowStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
// --- End: shard/syntax/nodes/Statements/ThrowStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/NullableTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

namespace shard
{
	class SHARD_API NullableTypeSyntax : public TypeSyntax
	{
	public:
		TypeSyntax* UnderlayingType = nullptr;
		SyntaxToken QuestionToken;

		inline NullableTypeSyntax(TypeSyntax* underlaying, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::NullableType, parent), UnderlayingType(underlaying) { }

		inline NullableTypeSyntax(const NullableTypeSyntax& other) = delete;

		inline virtual ~NullableTypeSyntax()
		{
			if (UnderlayingType != nullptr)
				delete UnderlayingType;
		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/NullableTypeSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)
// #include "shard/syntax/nodes/TypeDeclarationSyntax.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API ClassDeclarationSyntax : public TypeDeclarationSyntax
	{
	public:
		inline ClassDeclarationSyntax(SyntaxNode *const parent)
			: TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent) { }

		inline ClassDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : TypeDeclarationSyntax(SyntaxKind::ClassDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline ClassDeclarationSyntax(const ClassDeclarationSyntax& other) = delete;

		inline virtual ~ClassDeclarationSyntax()
		{

		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h ---

// --- Begin: shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline BinaryExpressionSyntax(const SyntaxToken operatorToken, SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::BinaryExpression, parent), OperatorToken(operatorToken) {}

		inline BinaryExpressionSyntax(const BinaryExpressionSyntax&) = delete;

		inline virtual ~BinaryExpressionSyntax()
		{
			delete Left;
			delete Right;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/Loops/ForStatementSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API ForStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken FirstSemicolon;
		SyntaxToken SecondSemicolon;
		SyntaxToken CloseCurlToken;

		StatementSyntax* InitializerStatement = nullptr;
		ExpressionSyntax* ConditionExpression = nullptr;
		StatementSyntax* AfterRepeatStatement = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline ForStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::ForStatement, parent) { }

		inline ForStatementSyntax(const ForStatementSyntax&) = delete;

		inline virtual ~ForStatementSyntax()
		{
			if (InitializerStatement != nullptr)
				delete InitializerStatement;

			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (AfterRepeatStatement != nullptr)
				delete AfterRepeatStatement;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
// --- End: shard/syntax/nodes/Loops/ForStatementSyntax.h ---

// --- Begin: shard/syntax/nodes/Types/PredefinedTypeSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

namespace shard
{
	class SHARD_API PredefinedTypeSyntax : public TypeSyntax
	{
	public:
		const SyntaxToken TypeToken;

		inline PredefinedTypeSyntax(const SyntaxToken typeToken, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::PredefinedType, parent), TypeToken(typeToken) { }

		inline PredefinedTypeSyntax(const PredefinedTypeSyntax& other) = delete;

		inline virtual ~PredefinedTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
// --- End: shard/syntax/nodes/Types/PredefinedTypeSyntax.h ---

// --- Begin: shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

namespace shard
{
	class SHARD_API TernaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken QuestionToken;
		SyntaxToken ColonToken;
		ExpressionSyntax* Condition = nullptr;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline TernaryExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::TernaryExpression, parent) { }

		inline TernaryExpressionSyntax(const TernaryExpressionSyntax&) = delete;

		inline virtual ~TernaryExpressionSyntax()
		{
			if (Condition != nullptr)
				delete Condition;

			if (Left != nullptr)
				delete Left;

			if (Right != nullptr)
				delete Right;
		}
	};
}
// --- End: shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h ---

// --- Begin: shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

namespace shard
{
	class SHARD_API DelegateDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken DelegateToken;
		SyntaxToken Semicolon;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateDeclarationSyntax(SyntaxNode *const parent)
			: MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

		inline DelegateDeclarationSyntax(const DelegateDeclarationSyntax& other) = delete;

		inline DelegateDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode *const parent) : MemberDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent)
		{
			Modifiers = info.Modifiers;
			DelegateToken = info.DeclareType;
			ReturnType = info.ReturnType;
			IdentifierToken = info.Identifier;
			TypeParameters = info.Generics;
		}

		inline virtual ~DelegateDeclarationSyntax()
		{
			if (ReturnType != nullptr)
				delete ReturnType;

			if (Params != nullptr)
				delete Params;
		}
	};
}
// --- End: shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h ---

// --- Begin: shard/SyntaxVisitor.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/SyntaxTree.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SymbolTable.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeParametersListSyntax.h" (Merged)

// #include "shard/syntax/nodes/Directives/UsingDirectiveSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ExpressionStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ConditionalClauseSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ReturnStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ThrowStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/BreakStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ContinueStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/Loops/WhileStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/ForStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/UntilStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h" (Merged)

// #include "shard/syntax/nodes/Types/PredefinedTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/GenericTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/ArrayTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/NullableTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/DelegateTypeSyntax.h" (Merged)

namespace shard
{
	class SHARD_API SyntaxVisitor
	{
    protected:
        shard::SymbolTable* Table;
        shard::NamespaceTree* Namespaces;
        shard::DiagnosticsContext& Diagnostics;

        inline SyntaxVisitor(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
            : Table(model.Table), Namespaces(model.Namespaces), Diagnostics(diagnostics) { }

        template<typename T>
        inline T *const LookupSymbol(shard::SyntaxNode *const node)
        {
            return static_cast<T *const>(Table->LookupSymbol(node));
        }

	public:
        virtual void VisitSyntaxTree(shard::SyntaxTree& tree);
        virtual void VisitCompilationUnit(shard::CompilationUnitSyntax *const node);
        virtual void VisitUsingDirective(shard::UsingDirectiveSyntax *const node);

        virtual void VisitTypeDeclaration(shard::MemberDeclarationSyntax *const node);
        virtual void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node);
        virtual void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node);
        virtual void VisitStructDeclaration(shard::StructDeclarationSyntax *const node);
        virtual void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node);

        virtual void VisitMemberDeclaration(shard::MemberDeclarationSyntax *const node);
        virtual void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node);
        virtual void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node);
        virtual void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node);
        virtual void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node);
        virtual void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node);
		virtual void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node);

        virtual void VisitStatementsBlock(shard::StatementsBlockSyntax *const node);
        virtual void VisitStatement(shard::StatementSyntax *const node);
        virtual void VisitExpressionStatement(shard::ExpressionStatementSyntax *const node);
        virtual void VisitVariableStatement(shard::VariableStatementSyntax *const node);
        virtual void VisitReturnStatement(shard::ReturnStatementSyntax *const node);
        virtual void VisitThrowStatement(shard::ThrowStatementSyntax *const node);
        virtual void VisitBreakStatement(shard::BreakStatementSyntax *const node);
        virtual void VisitContinueStatement(shard::ContinueStatementSyntax *const node);

        virtual void VisitWhileStatement(shard::WhileStatementSyntax *const node);
        virtual void VisitForStatement(shard::ForStatementSyntax *const node);
        virtual void VisitUntilStatement(shard::UntilStatementSyntax *const node);

        virtual void VisitConditionalClause(shard::ConditionalClauseBaseSyntax *const node);
        virtual void VisitIfStatement(shard::IfStatementSyntax *const node);
        virtual void VisitUnlessStatement(shard::UnlessStatementSyntax *const node);
        virtual void VisitElseStatement(shard::ElseStatementSyntax *const node);

        virtual void VisitExpression(shard::ExpressionSyntax *const node);
        virtual void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node);
        virtual void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node);
        virtual void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node);
        virtual void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node);
        virtual void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node);
        virtual void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node);
        virtual void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node);

        virtual void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node);
        virtual void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node);
        virtual void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node);

        virtual void VisitArgumentsList(shard::ArgumentsListSyntax *const node);
        virtual void VisitArgument(shard::ArgumentSyntax *const node);
        virtual void VisitParametersList(shard::ParametersListSyntax *const node);
        virtual void VisitParameter(shard::ParameterSyntax *const node);
        virtual void VisitIndexatorList(shard::IndexatorListSyntax *const node);
        virtual void VisitTypeParametersList(shard::TypeParametersListSyntax *const node);
        virtual void VisitTypeArgumentsList(shard::TypeArgumentsListSyntax *const node);

        virtual void VisitType(shard::TypeSyntax *const node);
        virtual void VisitPredefinedType(shard::PredefinedTypeSyntax *const node);
        virtual void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax *const node);
        virtual void VisitArrayType(shard::ArrayTypeSyntax *const node);
        virtual void VisitNullableType(shard::NullableTypeSyntax *const node);
        virtual void VisitGenericType(shard::GenericTypeSyntax *const node);
        virtual void VisitDelegateType(shard::DelegateTypeSyntax *const node);
	};
}
// --- End: shard/SyntaxVisitor.h ---

// --- Begin: shard/Vector.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

#include <vector>

namespace shard
{
	/*
	template<typename T>
	class SHARD_API Vector
	{
		std::vector<T> myVector;

	public:
		Vector() = default;

		size_t Length();
		size_t Capacity();
		void Shrink();
		void Reserve(size_t newCapacity);

		bool IsEmpty();
		void Clear();

		T& ElementAt(size_t index);
		const T& ElementAt(size_t index) const;

		T& operator[](size_t index);
		const T& operator[](size_t index) const;

		T& First();
		const T& First() const;

		T& Last();
		const T& Last() const;

		T& Append(const T& value);
		T& Append(T&& value);
		T& Append(std::initializer_list<T> values);

		template<typename... Args>
		T& EmplaceAppend(Args&&... args);

		void Insert(size_t index, const T& value);
		void Insert(size_t index, T&& value);

		T RemoveFirst();
		T RemoveLast();

		T* RawData();
		const T* RawData() const;
	};
	*/
}
// --- End: shard/Vector.h ---

// --- Begin: shard/compilation/ByteCodeEncoder.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/ArrayTypeSymbol.h" (Merged)

namespace shard
{
	class SHARD_API ByteCodeEncoder
	{
	public:
		inline ByteCodeEncoder() { }

		static void AppendData(std::vector<std::byte>& code, const void* data, size_t size);
		static void PasteData(std::vector<std::byte>& code, size_t at, const void* data, size_t size);

		void EmitNop(std::vector<std::byte>& code);
		void EmitHalt(std::vector<std::byte>& code);

		void EmitLoadConstNull(std::vector<std::byte>& code);
		void EmitLoadConstBool(std::vector<std::byte>& code, bool value);
		void EmitLoadConstInt64(std::vector<std::byte>& code, int64_t value);
		void EmitLoadConstDouble64(std::vector<std::byte>& code, double value);
		void EmitLoadConstChar16(std::vector<std::byte>& code, wchar_t value);
		void EmitLoadConstString(std::vector<std::byte>& code, std::vector<std::byte>& data, const wchar_t* value);

		void EmitLoadVarible(std::vector<std::byte>& code, uint16_t index);
		void EmitStoreVarible(std::vector<std::byte>& code, uint16_t index);

		void EmitJump(std::vector<std::byte>& code, size_t jump);
		void EmitJumpTrue(std::vector<std::byte>& code, size_t jump);
		void EmitJumpFalse(std::vector<std::byte>& code, size_t jump);
		void EmitReturn(std::vector<std::byte>& code);
		void EmitThrow(std::vector<std::byte>& code);

		void EmitMathAdd(std::vector<std::byte>& code);
		void EmitMathSub(std::vector<std::byte>& code);
		void EmitMathMult(std::vector<std::byte>& code);
		void EmitMathDiv(std::vector<std::byte>& code);
		void EmitMathMod(std::vector<std::byte>& code);
		void EmitMathPow(std::vector<std::byte>& code);

		void EmitCompareEqual(std::vector<std::byte>& code);
		void EmitCompareNotEqual(std::vector<std::byte>& code);
		void EmitCompareGreater(std::vector<std::byte>& code);
		void EmitCompareGreaterOrEqual(std::vector<std::byte>& code);
		void EmitCompareLess(std::vector<std::byte>& code);
		void EmitCompareLessOrEqual(std::vector<std::byte>& code);
		void EmitCompareNot(std::vector<std::byte>& code);

		void EmitNewObject(std::vector<std::byte>& code, TypeSymbol* type);
		void EmitLoadField(std::vector<std::byte>& code, FieldSymbol* type);
		void EmitStoreField(std::vector<std::byte>& code, FieldSymbol* type);

		void EmitNewArray(std::vector<std::byte>& code, ArrayTypeSymbol* type);
		void EmitLoadArrayElement(std::vector<std::byte>& code);
		void EmitStoreArrayElement(std::vector<std::byte>& code);

		void EmitLoadStaticField(std::vector<std::byte>& code, FieldSymbol* type);
		void EmitStoreStaticField(std::vector<std::byte>& code, FieldSymbol* type);

		void EmitCallMethodSymbol(std::vector<std::byte>& code, MethodSymbol* method);
		//void EmitCallFunction(std::vector<std::byte>& code, MethodSymbolDelegate* func);
	};
}
// --- End: shard/compilation/ByteCodeEncoder.h ---

// --- Begin: shard/compilation/ProgramVirtualImage.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <vector>

namespace shard
{
	class SHARD_API ProgramVirtualImage
	{
	public:
		const SemanticModel& SemModel;
		MethodSymbol* EntryPoint = nullptr;
		std::vector<std::byte> DataSection;

		inline ProgramVirtualImage(SemanticModel& semanticModel)
			: SemModel(semanticModel) { }

		inline ProgramVirtualImage(const ProgramVirtualImage& other) = delete;
	};
}
// --- End: shard/compilation/ProgramVirtualImage.h ---

// --- Begin: shard/compilation/AbstractEmiter.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/compilation/ProgramVirtualImage.h" (Merged)
// #include "shard/compilation/ByteCodeEncoder.h" (Merged)

// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/parsing/SyntaxTree.h" (Merged)

// #include "shard/SyntaxVisitor.h" (Merged)

// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h" (Merged)

// #include "shard/syntax/nodes/Loops/ForStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/UntilStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/WhileStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/BreakStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ConditionalClauseSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ContinueStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ExpressionStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ReturnStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ThrowStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <vector>
#include <stack>

namespace shard
{
	// basically, a compiler
	class SHARD_API AbstractEmiter : SyntaxVisitor
	{
        struct LoopScope
        {
            size_t LoopStart; // Address of first OpCode of loop, used for 'looping jump'
            size_t BlockEnd; // Addredd of OpCode right after last OpCode of looping block, used for 'continue' statement
            size_t LoopEnd; // Address of OpCode right after last OpCode of entire loop, used for 'looping exit', or 'break' statement

            std::vector<size_t> BlockEndBacktracks;
            std::vector<size_t> LoopEndBacktracks;
        };

        shard::ByteCodeEncoder Encoder;
        shard::MethodSymbol* GeneratingFor = nullptr;
		shard::ProgramVirtualImage& Program;
		std::vector<shard::MethodSymbol*> EntryPointCandidates;
        std::stack<LoopScope> Loops;

	public:
		inline AbstractEmiter(shard::ProgramVirtualImage& program, shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics), Program(program), Encoder() { }

        void VisitSyntaxTree(shard::SyntaxTree& tree) override;

        void VisitArgumentsList(ArgumentsListSyntax* node) override;

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;

        void VisitExpressionStatement(shard::ExpressionStatementSyntax *const node) override;
        void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;
        void VisitReturnStatement(shard::ReturnStatementSyntax *const node) override;
        void VisitThrowStatement(shard::ThrowStatementSyntax *const node) override;
        void VisitBreakStatement(shard::BreakStatementSyntax *const node) override;
        void VisitContinueStatement(shard::ContinueStatementSyntax *const node) override;

        void VisitWhileStatement(shard::WhileStatementSyntax *const node) override;
        void VisitForStatement(shard::ForStatementSyntax *const node) override;
        void VisitUntilStatement(shard::UntilStatementSyntax *const node) override;

        void VisitConditionalClause(shard::ConditionalClauseBaseSyntax *const node) override;
        void VisitIfStatement(shard::IfStatementSyntax *const node) override;
        void VisitUnlessStatement(shard::UnlessStatementSyntax *const node) override;
        void VisitElseStatement(shard::ElseStatementSyntax *const node) override;

        void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node) override;
        void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node) override;
        void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node) override;
        void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;
        void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node) override;
        void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node) override;
        void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node) override;

        void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node) override;
        void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node) override;
        void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node) override;
	};
}
// --- End: shard/compilation/AbstractEmiter.h ---

// --- Begin: shard/compilation/OperationCode.h ---
#include <cstdint>

namespace shard
{
	enum class OpCode : uint16_t
	{
		/// <summary>
		/// Upon receiving this byte, VM does nothing.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Nop = 1,

		/// <summary>
		/// VM immidietly stops execution upon receiving this byte.
		/// <para>Includes 1 parameter :</para>
		/// <para>> int32_t exit_code - exit code of virtual program image
		/// </summary>
		Halt,

		/// <summary>
		/// Pops current top value of stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		PopStack,

		/// <summary>
		/// Pops N values from stack. Used for stack cleanup
		/// <para>Includes 1 parameter :</para>
		/// <para>> uin8_t Value - How mush values to pop.</para>
		/// </summary>
		PopStack_N,

		/// <summary>
		/// Loads `null` and pushes its ObjectInstance to stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LoadConst_Null,

		/// <summary>
		/// Loads `boolean` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> bool Value - bool stored to load.</para>
		/// </summary>
		LoadConst_Boolean,

		/// <summary>
		/// Loads `signed 64-bit integer number` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> long Value - long stored to load.</para>
		/// </summary>
		LoadConst_Integer64,

		/// <summary>
		/// Loads `signed 64-bit floating point integer`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> double Value - double stored to load.</para>
		/// </summary>
		LoadConst_Rational64,

		/// <summary>
		/// Loads `UTF16 character`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> wchar_t Value - UTF16 character stored to load.</para>
		/// </summary>
		LoadConst_Char,

		/// <summary>
		/// Loads NULL-terminated UTF16 string from data section, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Value - offset inside Data section of program, to first symbol of NULL-Terminated UTF16 string to load.</para>
		/// </summary>
		LoadConst_String,

		/// <summary>
		/// Pops ObjectInstance* from stack and stores it to variable slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> uint16_t Value - Zero-based index of variable, where instance will be written to.</para>
		/// </summary>
		StoreVariable,

		/// <summary>
		/// Pushes copy of ObjectInstance* from variable slot on given index to stack top.
		/// <para>Includes 1 parameter :</para>
		/// <para>> uint16_t Value - Zero-based index of variable, where instance will be readed from.</para>
		/// </summary>
		LoadVariable,

		/// <summary>
		/// Unconditional jump to a relative offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains True.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump_True,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains False.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump_False,

		/// <summary>
		/// Returns from the current method call, destroying the current CallStackFrame.
		/// If the method is non-void, the top of the stack is treated as the return value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Return,

		/// <summary>
		/// Interrupts current method flow with an exception.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Throw,

		Math_Addition,
		Math_Substraction,
		Math_Multiplication,
		Math_Division,
		Math_Module,
		Math_Power,

		/// <summary>
		/// Pops two values, compares them for equality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Equal,

		/// <summary>
		/// Pops two values, compares them for inequality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_NotEqual,

		/// <summary>
		/// Pops two values, checks if value B is greater than value A (B > A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Greater,

		/// <summary>
		/// Pops two values, checks if value B is greater or equal to value A (B >= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_GreaterOrEqual,

		/// <summary>
		/// Pops two values, checks if value B is less than value A (B < A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Less,

		/// <summary>
		/// Pops two values, checks if value B is less or equal to value A (B <= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_LessOrEqual,

		/// <summary>
		/// Logical NOT operation. Pops boolean/integer, pushes inverted value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Not,

		/// <summary>
        /// Creates a new instance of a class/struct defined by TypeSymbol.
        /// <para>Includes 2 parameters :</para>
        /// <para>> TypeSymbol* pType - The type to instantiate.</para>
        /// <para>> ConstructorSymbol* pCtor - The Constructor to invoke.</para>
        /// </summary>
        NewObject,

        /// <summary>
        /// Pops an object instance from the stack, loads a field value from it and pushes gotten instance to stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LoadField,

        /// <summary>
        /// Pops a value and an object instance, then stores the value into the object's field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        StoreField,

		/// <summary>
        /// Creates a new array of specified type and size.
        /// Pops size (int) from stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> TypeSymbol* pElementType - Type of array elements.</para>
        /// </summary>
        NewArray,

        /// <summary>
        /// Loads an element from an array. Pops index and array reference. Pushes element.
        /// <para>Includes no additional parameters.</para>
        /// </summary>
        LoadArrayElement,

        /// <summary>
        /// Stores a value into an array. Pops value, index, and array reference.
        /// <para>Includes no additional parameters.</para>
        /// </summary>
        StoreArrayElement,

        /// <summary>
        /// Loads a static field value.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LoadStaticField,

        /// <summary>
        /// Pops a value, then stores the value into the static field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        StoreStaticField,

		/// <summary>
		/// Invokes MethodSymbol with creation of new CallStackFrame.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// </summary>
		CallMethodSymbol
	};
}
// --- End: shard/compilation/OperationCode.h ---

// --- Begin: shard/compilation/ByteCodeDecoder.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/compilation/OperationCode.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)
// #include "shard/syntax/symbols/ArrayTypeSymbol.h" (Merged)

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace shard
{
	class SHARD_API ByteCodeDecoder
	{
		const std::vector<std::byte>& _code;
		size_t _ip = 0;

	public:
		inline ByteCodeDecoder(const std::vector<std::byte>& code) : _code(code) { }

		bool IsEOF();
		size_t Index() const;
		void SetCursor(fpos_t amount);
		OpCode AbsorbOpCode();

		bool AbsorbBoolean();
		int64_t AbsorbInt64();
		double AbsorbDouble64();
		wchar_t AbsorbChar16();
		size_t AbsorbString();

		uint16_t AbsorbVariableSlot();
		size_t AbsorbJump();

		TypeSymbol* AbsorbTypeSymbol();
		FieldSymbol* AbsorbFieldSymbol();
		ArrayTypeSymbol* AbsorbArraySymbol();
		MethodSymbol* AbsorbMethodSymbol();
		ConstructorSymbol* AbsorbConstructorSymbol();
		MethodSymbolDelegate AbsorbFunctionPtr();
	};
}
// --- End: shard/compilation/ByteCodeDecoder.h ---

// --- Begin: shard/parsing/LayoutGenerator.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

namespace shard
{
	class SHARD_API LayoutGenerator
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		LayoutGenerator(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Generate(shard::SemanticModel& semanticModel);

	private:
		void FixObjectLayout(shard::SemanticModel& semanticModel, shard::TypeSymbol* objectInfo);
	};
}
// --- End: shard/parsing/LayoutGenerator.h ---

// --- Begin: shard/parsing/SemanticAnalyzer.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/SyntaxTree.h" (Merged)

namespace shard
{
	class SHARD_API SemanticAnalyzer
	{
		shard::DiagnosticsContext& Diagnostics;

	public:
		inline SemanticAnalyzer(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void Analyze(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel);
	};
}
// --- End: shard/parsing/SemanticAnalyzer.h ---

// --- Begin: shard/parsing/lexical/SourceProvider.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/TokenType.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API SourceProvider
	{
	public:
		virtual shard::SyntaxToken Current() = 0;
		virtual shard::SyntaxToken Consume() = 0;
		virtual shard::SyntaxToken Peek(int index = 0) = 0;

		virtual bool CanConsume() = 0;
		virtual bool CanPeek() = 0;
	};
}
// --- End: shard/parsing/lexical/SourceProvider.h ---

// --- Begin: shard/parsing/SourceParser.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/lexical/SourceProvider.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/MemberDeclarationInfo.h" (Merged)
// #include "shard/parsing/SyntaxTree.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/TokenType.h" (Merged)
// #include "shard/syntax/SyntaxNode.h" (Merged)

// #include "shard/syntax/nodes/TypeDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)

// #include "shard/syntax/nodes/Directives/UsingDirectiveSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/ConditionalClauseSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ReturnStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ThrowStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/BreakStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ContinueStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h" (Merged)

// #include "shard/syntax/nodes/Loops/ForStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/WhileStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/UntilStatementSyntax.h" (Merged)

#include <initializer_list>
#include <vector>

namespace shard
{
	// Note that this parser is only capable of contextual parsing, and only should be used to parse full compulation units. DO NOT try to parse individual members or expression with this parser out of stream
	class SHARD_API SourceParser
	{
	private:
		shard::DiagnosticsContext& Diagnostics;

	public:
		SourceParser(shard::DiagnosticsContext& diagnostics)
			: Diagnostics(diagnostics) { }

		void FromSourceProvider(shard::SyntaxTree& syntaxTree, shard::SourceProvider& reader);

		// 1. top tier components
		shard::CompilationUnitSyntax *const ReadCompilationUnit(shard::SourceProvider& reader);
		shard::UsingDirectiveSyntax *const ReadUsingDirective(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::NamespaceDeclarationSyntax *const ReadNamespaceDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 2. Type declarations
		shard::MemberDeclarationSyntax *const ReadMemberDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ClassDeclarationSyntax *const ReadClassDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::StructDeclarationSyntax *const ReadStructDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		//shard::InterfaceDelcarationSyntax *const ReadInterfaceDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::DelegateDeclarationSyntax *const ReadDelegateDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);

		std::vector<shard::SyntaxToken> ReadMemberModifiers(shard::SourceProvider& reader);
		void ReadTypeBody(shard::SourceProvider& reader, shard::TypeDeclarationSyntax *const syntax);
		//shard::TypeDeclarationSyntax *const make_type(shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);

		// 3. Type members
		shard::ConstructorDeclarationSyntax *const ReadConstructorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::MethodDeclarationSyntax *const ReadMethodDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::FieldDeclarationSyntax *const ReadFieldDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::PropertyDeclarationSyntax *const ReadPropertyDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::IndexatorDeclarationSyntax *const ReadIndexatorDeclaration(shard::SourceProvider& reader, shard::MemberDeclarationInfo& info, shard::SyntaxNode *const parent);
		shard::AccessorDeclarationSyntax *const ReadAccessorDeclaration(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 4. Code blocks
		shard::StatementsBlockSyntax *const ReadStatementsBlock(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::StatementSyntax *const ReadStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 5. Keywords and statements
		shard::KeywordStatementSyntax *const ReadKeywordStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ReturnStatementSyntax *const ReadReturnStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ThrowStatementSyntax *const ReadThrowStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::BreakStatementSyntax *const ReadBreakStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ContinueStatementSyntax *const ReadContinueStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 6. Lexical structures
		shard::ConditionalClauseBaseSyntax *const ReadConditionalClause(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::WhileStatementSyntax *const ReadWhileStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::UntilStatementSyntax *const ReadUntilStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ForStatementSyntax *const ReadForStatement(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 7. Expression
		shard::ExpressionSyntax *const ReadExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent, int bindingPower);
		shard::ExpressionSyntax *const ReadNullDenotation(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ExpressionSyntax *const ReadLeftDenotation(shard::SourceProvider& reader, shard::SyntaxNode *const parent, shard::ExpressionSyntax *const leftExpr);

		shard::CollectionExpressionSyntax *const ReadCollectionExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ObjectExpressionSyntax *const ReadObjectExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TernaryExpressionSyntax *const ReadTernaryExpression(shard::SourceProvider& reader, shard::ExpressionSyntax *const condition, shard::SyntaxNode *const parent);

		shard::LambdaExpressionSyntax *const ReadLambdaExpression(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::LinkedExpressionNode *const ReadLinkedExpressionNode(shard::SourceProvider& reader, shard::SyntaxNode *const parent, shard::ExpressionSyntax *const lastNode, bool isFirst);

		shard::ArgumentsListSyntax *const ReadArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::IndexatorListSyntax *const ReadIndexatorList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ParametersListSyntax *const ReadIndexerParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::ParametersListSyntax *const ReadParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeParametersListSyntax *const ReadTypeParametersList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeArgumentsListSyntax *const ReadTypeArgumentsList(shard::SourceProvider& reader, shard::SyntaxNode *const parent);

		// 8. Other
		shard::TypeSyntax *const ReadType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadIdentifierNameType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadDelegateType(shard::SourceProvider& reader, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadModifiedType(shard::SourceProvider& reader, shard::TypeSyntax *const type, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadArrayType(shard::SourceProvider& reader, shard::TypeSyntax *const type, shard::SyntaxNode *const parent);
		shard::TypeSyntax *const ReadGenericType(shard::SourceProvider& reader, shard::TypeSyntax *const previous, shard::SyntaxNode *const parent);

	private:
		// Fourth layer - lexing helpers
		shard::SyntaxToken Expect(shard::SourceProvider& reader, shard::TokenType kind, const wchar_t* message);
		bool Matches(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types);
		bool TryMatch(shard::SourceProvider& reader, std::initializer_list<shard::TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);
	};
}
// --- End: shard/parsing/SourceParser.h ---

// --- Begin: shard/parsing/lexical/reading/SourceTextProvider.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API SourceTextProvider
	{
	public:
		virtual bool ReadNext(wchar_t& ch) = 0;
		virtual bool PeekNext(wchar_t& ch) = 0;
		virtual std::wstring& GetName() = 0;
	};
}
// --- End: shard/parsing/lexical/reading/SourceTextProvider.h ---

// --- Begin: shard/parsing/lexical/LexicalAnalyzer.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/TokenType.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)
// #include "shard/parsing/lexical/SourceProvider.h" (Merged)
// #include "shard/parsing/lexical/reading/SourceTextProvider.h" (Merged)

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API LexicalAnalyzer : public SourceProvider
	{
	protected:
		int Line;
		int Offset;
		wchar_t Symbol;
		wchar_t PeekSymbol;

		bool ownsSourceTextProvider = false;
		SourceTextProvider* SourceText;
		std::deque<shard::SyntaxToken> ReadBuffer;

	public:
		LexicalAnalyzer(SourceTextProvider& sourceText);
		LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider);
		~LexicalAnalyzer();

		shard::SyntaxToken Current() override;
		shard::SyntaxToken Consume() override;
		shard::SyntaxToken Peek(int index = 0) override;

		bool CanConsume() override;
		bool CanPeek() override;

	protected:
		shard::TextLocation GetLocation(std::wstring& word);
		bool ReadNextToken(shard::SyntaxToken& pToken);

		bool ReadNextReal();
		bool ReadNextWord(std::wstring& word, shard::TokenType& type);
		bool ReadNextWhileAlpha(std::wstring& word);

		bool ReadNumberLiteral(std::wstring& word, shard::TokenType& type);
		bool ReadCharLiteral(std::wstring& word, bool notEcran, bool& wasClosed);
		bool ReadStringLiteral(std::wstring& word, bool notEcran, bool& wasClosed);

		bool IsNumberLiteral(shard::TokenType& type) const;
		bool IsCharLiteral(shard::TokenType& type, bool& dontEcran);
		bool IsStringLiteral(shard::TokenType& type, bool& dontEcran);

		bool IsPunctuation(std::wstring& word, shard::TokenType& type);
		bool IsOperator(std::wstring& word, shard::TokenType& type);
		bool IsWordOperator(std::wstring& word, shard::TokenType& type);
		bool IsNullLiteral(std::wstring& word, shard::TokenType& type);
		bool IsBooleanLiteral(std::wstring& word, shard::TokenType& type);
		bool IsModifier(std::wstring& word, shard::TokenType& type);
		bool IsDirectiveDecl(std::wstring& word, shard::TokenType& type);
		bool IsTypeDecl(std::wstring& word, shard::TokenType& type);
		bool IsType(std::wstring& word, shard::TokenType& type);
		bool IsKeyword(std::wstring& word, shard::TokenType& type);
		bool IsLoopKeyword(std::wstring& word, shard::TokenType& type);
		bool IsConditionalKeyword(std::wstring& word, shard::TokenType& type);
		bool IsFunctionalKeyword(std::wstring& word, shard::TokenType& type);
	};
}
// --- End: shard/parsing/lexical/LexicalAnalyzer.h ---

// --- Begin: shard/parsing/lexical/LexicalBuffer.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/lexical/reading/SourceTextProvider.h" (Merged)
// #include "shard/parsing/lexical/SourceProvider.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)
// #include "shard/syntax/SyntaxToken.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API LexicalBuffer : public SourceProvider
	{
	private:
		std::vector<shard::SyntaxToken> Sequence;
		size_t CurrentIndex;

	public:
		LexicalBuffer();
		virtual ~LexicalBuffer();

		static LexicalBuffer From(SourceProvider& provider);
		static LexicalBuffer From(SourceTextProvider& reader);
		static LexicalBuffer From(std::vector<shard::SyntaxToken> fromvector);

		void PopulateFrom(SourceProvider& reader);
		void PopulateFrom(SourceTextProvider& reader);
		void PopulateFrom(std::vector<shard::SyntaxToken> fromvector);

		void SetSequence(std::vector<shard::SyntaxToken> setvector);
		void SetIndex(size_t newIndex);

		size_t Size();
		shard::SyntaxToken At(size_t index);
		void Push(shard::SyntaxToken token);
		void Clear();

		shard::SyntaxToken Front();
		shard::SyntaxToken Back();

		std::vector<shard::SyntaxToken>::iterator begin();
		std::vector<shard::SyntaxToken>::iterator end();

		std::vector<shard::SyntaxToken>::const_iterator begin() const;
		std::vector<shard::SyntaxToken>::const_iterator end() const;

		shard::SyntaxToken Current() override;
		shard::SyntaxToken Consume() override;
		shard::SyntaxToken Peek(int index) override;
		bool CanConsume() override;
		bool CanPeek() override;
	};
}

// --- End: shard/parsing/lexical/LexicalBuffer.h ---

// --- Begin: shard/parsing/lexical/reading/FileReader.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/lexical/reading/SourceTextProvider.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)

#include <fstream>
#include <string>

namespace shard
{
	class SHARD_API FileReader : public SourceTextProvider
	{
		std::wstring Filename;
		std::wfstream InputStream;

	public:
		FileReader(const std::wstring& fileName);
		virtual ~FileReader();

		bool ReadNext(wchar_t& ch) override;
		bool PeekNext(wchar_t& ch) override;
		std::wstring& GetName() override;
	};
}
// --- End: shard/parsing/lexical/reading/FileReader.h ---

// --- Begin: shard/parsing/lexical/reading/StringStreamReader.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/lexical/reading/SourceTextProvider.h" (Merged)
// #include "shard/parsing/analysis/TextLocation.h" (Merged)

#include <sstream>
#include <string>

namespace shard
{
	class SHARD_API StringStreamReader : public SourceTextProvider
	{
		std::wstring name;
		std::wstringstream stringStream;

	public:
		StringStreamReader(const std::wstring& name, std::wstringstream& source);
		StringStreamReader(const std::wstring& name, const std::wstring& source);
		StringStreamReader(const std::wstring& name, const wchar_t* source, size_t size);
		virtual ~StringStreamReader();

		bool ReadNext(wchar_t& ch) override;
		bool PeekNext(wchar_t& ch) override;
		std::wstring& GetName() override;
	};
}
// --- End: shard/parsing/lexical/reading/StringStreamReader.h ---

// --- Begin: shard/parsing/semantic/visiting/ScopeVisitor.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/semantic/SymbolTable.h" (Merged)
// #include "shard/parsing/semantic/SemanticScope.h" (Merged)
// #include "shard/parsing/semantic/NamespaceTree.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/NamespaceSymbol.h" (Merged)

#include <stack>

namespace shard
{
	class SHARD_API ScopeVisitor
	{
		std::stack<shard::SemanticScope*> scopeStack;

	protected:
		inline ScopeVisitor(shard::SymbolTable *const symbolTable)
		{
			scopeStack.push(shard::SymbolTable::Global::Scope);
		}

		shard::SemanticScope *const CurrentScope();
		void PushScope(shard::SyntaxSymbol *const symbol);
		void PopScope();

		virtual void Declare(shard::SyntaxSymbol *const symbol);

		virtual bool CheckNameDeclared(const std::wstring& name);
		virtual bool CheckSymbolNameDeclared(shard::SyntaxSymbol *const symbol);

		shard::SyntaxSymbol *const OwnerSymbol();
		shard::TypeSymbol *const OwnerType();
		//shard::NamespaceSymbol* OwnerNamespace();
		//shard::NamespaceNode* OwnerNamespaceNode();
		MethodSymbol *const FindHostMethodSymbol();

		bool IsSymbolAccessible(shard::SyntaxSymbol *const symbol);
	};
}
// --- End: shard/parsing/semantic/visiting/ScopeVisitor.h ---

// --- Begin: shard/parsing/semantic/visiting/DeclarationCollector.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/SyntaxVisitor.h" (Merged)
// #include "shard/parsing/semantic/visiting/ScopeVisitor.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)

// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h" (Merged)

namespace shard
{
	class SHARD_API DeclarationCollector : public SyntaxVisitor, ScopeVisitor
	{
	protected:
		void Declare(shard::SyntaxSymbol *const symbol) override;

	public:
		inline DeclarationCollector(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node) override;

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;
	};
}
// --- End: shard/parsing/semantic/visiting/DeclarationCollector.h ---

// --- Begin: shard/syntax/symbols/TypeParameterSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/SymbolAccesibility.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API TypeParameterSymbol : public TypeSymbol
	{
	public:
		// Для type parameters без ограничений возвращаем Any
		// В будущем здесь можно добавить ограничения (constraints)
		//TypeSymbol* ConstraintType = nullptr;

		inline TypeParameterSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::TypeParameter)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline TypeParameterSymbol(const TypeParameterSymbol& other) = delete;

		inline virtual ~TypeParameterSymbol() override = default;
	};
}
// --- End: shard/syntax/symbols/TypeParameterSymbol.h ---

// --- Begin: shard/syntax/symbols/GenericTypeSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/PropertySymbol.h" (Merged)
// #include "shard/syntax/symbols/IndexatorSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeParameterSymbol.h" (Merged)

#include <string>
#include <vector>
#include <unordered_map>

namespace shard
{
	class SHARD_API GenericTypeSymbol : public TypeSymbol
	{
		std::unordered_map<TypeParameterSymbol*, TypeSymbol*> _typeParametersMap;

	public:
		TypeSymbol* UnderlayingType = nullptr;

		inline GenericTypeSymbol(TypeSymbol* underlayingType) : TypeSymbol(underlayingType->Name, SyntaxKind::GenericType), UnderlayingType(underlayingType)
		{
			IsReferenceType = underlayingType->IsReferenceType;
		}

		inline GenericTypeSymbol(const GenericTypeSymbol& other) = delete;

		inline virtual ~GenericTypeSymbol()
		{

		}

		void AddTypeParameter(TypeParameterSymbol* typeParam, TypeSymbol* constraintType);
		TypeSymbol* SubstituteTypeParameters(TypeParameterSymbol* typeParam);

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes) override;
		IndexatorSymbol* FindIndexator(std::vector<TypeSymbol*> parameterTypes) override;
		FieldSymbol* FindField(std::wstring& name) override;
		PropertySymbol* FindProperty(std::wstring& name) override;
	};
}
// --- End: shard/syntax/symbols/GenericTypeSymbol.h ---

// --- Begin: shard/parsing/semantic/visiting/ExpressionBinder.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/SyntaxVisitor.h" (Merged)
// #include "shard/parsing/semantic/visiting/ScopeVisitor.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SemanticScope.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/GenericTypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)
// #include "shard/syntax/symbols/IndexatorSymbol.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)
// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
// #include "shard/syntax/nodes/Directives/UsingDirectiveSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h" (Merged)

// #include "shard/syntax/nodes/Loops/ForStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/UntilStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/WhileStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/ConditionalClauseSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ReturnStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)

#include <unordered_map>
#include <vector>

namespace shard
{
	class SHARD_API ExpressionBinder : public SyntaxVisitor, ScopeVisitor
	{
		std::unordered_map<shard::ExpressionSyntax*, shard::TypeSymbol*> expressionTypes;

		bool GetIsStaticContext(const shard::ExpressionSyntax* expression);
		void SetExpressionType(shard::ExpressionSyntax* expression, shard::TypeSymbol* type);
		shard::TypeSymbol* GetExpressionType(shard::ExpressionSyntax* expression);
		shard::TypeSymbol* FindTargetReturnType(shard::SemanticScope*& scope);
		shard::TypeSymbol* ResolveLeftDenotation();

		shard::TypeSymbol* AnalyzeLiteralExpression(shard::LiteralExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeBinaryExpression(shard::BinaryExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeUnaryExpression(shard::UnaryExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeObjectExpression(shard::ObjectExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeCollectionExpression(shard::CollectionExpressionSyntax *const node);

		shard::TypeSymbol* AnalyzeMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzePropertyAccessExpression(shard::MemberAccessExpressionSyntax *const node, shard::PropertySymbol* property, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeFieldKeywordExpression(shard::MemberAccessExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeInvokationExpression(shard::InvokationExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeIndexatorExpression(shard::IndexatorExpressionSyntax *const node, shard::TypeSymbol* currentType);

		shard::ConstructorSymbol* ResolveConstructor(shard::ObjectExpressionSyntax *const node);
		shard::MethodSymbol* ResolveMethod(shard::InvokationExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::IndexatorSymbol* ResolveIndexator(shard::IndexatorExpressionSyntax *const node, shard::TypeSymbol* currentType);

		bool MatchMethodArguments(std::vector<ParameterSymbol*> parameters, std::vector<shard::ArgumentSyntax*> arguments, shard::GenericTypeSymbol* genericType = nullptr);
		shard::TypeSymbol* SubstituteTypeParameters(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericType);

		shard::TypeSymbol* AnalyzeNumberLiteral(shard::LiteralExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeDoubleLiteral(shard::LiteralExpressionSyntax *const node);

	public:
		inline ExpressionBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;

		void VisitWhileStatement(shard::WhileStatementSyntax *const node) override;
		void VisitUntilStatement(shard::UntilStatementSyntax *const node) override;
		void VisitForStatement(shard::ForStatementSyntax *const node) override;
		void VisitIfStatement(shard::IfStatementSyntax *const node) override;
		void VisitUnlessStatement(shard::UnlessStatementSyntax *const node) override;
		void VisitReturnStatement(shard::ReturnStatementSyntax *const node) override;

		void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node) override;
		void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node) override;
		void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node) override;
		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;
		void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node) override;
		void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node) override;
		void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node) override;

		void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node) override;
		void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node) override;
		void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node) override;
	};
}

// --- End: shard/parsing/semantic/visiting/ExpressionBinder.h ---

// --- Begin: shard/parsing/semantic/visiting/TypeBinder.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/SyntaxVisitor.h" (Merged)
// #include "shard/parsing/semantic/visiting/ScopeVisitor.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

// #include "shard/syntax/nodes/CompilationUnitSyntax.h" (Merged)
// #include "shard/syntax/nodes/TypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Directives/UsingDirectiveSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
//#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
//#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>

// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/ParametersListSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/ArrayTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/GenericTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/NullableTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/PredefinedTypeSyntax.h" (Merged)

namespace shard
{
	class SHARD_API TypeBinder : public SyntaxVisitor, ScopeVisitor
	{
	public:
		inline TypeBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;

		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;

		void VisitParameter(shard::ParameterSyntax *const node) override;

		void VisitPredefinedType(shard::PredefinedTypeSyntax *const node) override;
		void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax *const node) override;
		void VisitArrayType(shard::ArrayTypeSyntax *const node) override;
		void VisitNullableType(shard::NullableTypeSyntax *const node) override;
		void VisitGenericType(shard::GenericTypeSyntax *const node) override;
		void VisitDelegateType(shard::DelegateTypeSyntax *const node) override;
	};
}
// --- End: shard/parsing/semantic/visiting/TypeBinder.h ---

// --- Begin: shard/runtime/ObjectInstance.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)

#include <string>
#include <cstdint>

namespace shard
{
	class VirtualMachine;

	class SHARD_API ObjectInstance
	{
	public:
		const uint64_t Id;
		const shard::TypeSymbol* Info;
		const bool IsNullable = false;

		bool IsFieldInstance = false;
		size_t ReferencesCounter;
		void* Ptr;

		inline ObjectInstance(const uint64_t id, const shard::TypeSymbol* info, void* ptr)
			: Id(id), Info(info), Ptr(ptr), ReferencesCounter(0) { }

		inline ~ObjectInstance() = default;

		static ObjectInstance* FromValue(int64_t value);
		static ObjectInstance* FromValue(double value);
		static ObjectInstance* FromValue(bool value);
		static ObjectInstance* FromValue(wchar_t value);
		static ObjectInstance* FromValue(const wchar_t* value);
		static ObjectInstance* FromValue(const std::wstring& value);

		ObjectInstance* GetField(shard::FieldSymbol* field);
		void SetField(shard::FieldSymbol* field, ObjectInstance* instance);

		ObjectInstance* GetElement(size_t index);
		void SetElement(size_t index, ObjectInstance* instance);
		bool IsInBounds(size_t index);

		void IncrementReference();
		void DecrementReference();

		void WriteBoolean(const bool& value) const;
		void WriteInteger(const int64_t& value) const;
		void WriteDouble(const double& value) const;
		void WriteCharacter(const wchar_t& value) const;
		void WriteString(const std::wstring& value) const;

		bool AsBoolean() const;
		int64_t AsInteger() const;
		double AsDouble() const;
		wchar_t AsCharacter() const;
		std::wstring& AsString() const;

		void* OffsetMemory(const size_t offset, const size_t size) const;
		void ReadMemory(const size_t offset, const size_t size, void* dst) const;
		void WriteMemory(const size_t offset, const size_t size, const void* src) const;
	};
}
// --- End: shard/runtime/ObjectInstance.h ---

// --- Begin: shard/runtime/CallStackFrame.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/ObjectInstance.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <vector>

namespace shard
{
	class VirtualMachine;

	enum class SHARD_API FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
		LoopBreak,
		LoopContinue,
	};

	class SHARD_API CallStackFrame
	{
	public:
		const VirtualMachine* Host;
		CallStackFrame* PreviousFrame;
		TypeSymbol* WithinType;
		MethodSymbol* Method;

		std::vector<ObjectInstance*> EvalStack;
		std::vector<TypeSymbol*> TypeArguments;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;

		inline CallStackFrame(const VirtualMachine* host, CallStackFrame* previousFrame, TypeSymbol* withinType, MethodSymbol* method)
			: Host(host), WithinType(withinType), Method(method), PreviousFrame(previousFrame) { }

		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		void PushStack(ObjectInstance* value);
		ObjectInstance* PopStack();
		ObjectInstance* PeekStack();

		inline ~CallStackFrame()
		{
			Method = nullptr;
			PreviousFrame = nullptr;
		}
	};
}
// --- End: shard/runtime/CallStackFrame.h ---

// --- Begin: shard/runtime/ArgumentsSpan.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/ObjectInstance.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <vector>
#include <string>

namespace shard
{
	class SHARD_API ArgumentsSpan
	{
		const MethodSymbol* Method;
		const std::vector<ObjectInstance*> Span;

	public:
		inline ArgumentsSpan(const MethodSymbol* method, std::vector<ObjectInstance*> span)
			: Method(method), Span(span) { }

		inline ArgumentsSpan(const ArgumentsSpan& other) = delete;

		~ArgumentsSpan();

		ObjectInstance* operator[](int index) const;
		ObjectInstance* Find(const std::wstring& name) const;
	};
}
// --- End: shard/runtime/ArgumentsSpan.h ---

// --- Begin: shard/runtime/AbstractInterpreter.h ---
// TODO: rewrite
/*
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/CallStackFrame.h" (Merged)
// #include "shard/runtime/ArgumentsSpan.h" (Merged)
// #include "shard/runtime/ObjectInstance.h" (Merged)

// #include "shard/parsing/SyntaxTree.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

// #include "shard/syntax/nodes/ExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/StatementsBlockSyntax.h" (Merged)
// #include "shard/syntax/nodes/ArgumentsListSyntax.h" (Merged)

// #include "shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h" (Merged)
// #include "shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/ReturnStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ThrowStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/Loops/ForStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/UntilStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Loops/WhileStatementSyntax.h" (Merged)

// #include "shard/syntax/nodes/Statements/ConditionalClauseSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/VariableStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ExpressionStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/BreakStatementSyntax.h" (Merged)
// #include "shard/syntax/nodes/Statements/ContinueStatementSyntax.h" (Merged)

#include <stack>
#include <vector>
// #include "shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h" (Merged)

namespace shard
{
	class SHARD_API AbstractInterpreter
	{
	private:
		static std::stack<CallStackFrame*> callStack;

	public:
		static CallStackFrame* CurrentFrame();
		static void PushFrame(const shard::MethodSymbol* methodSymbol, const shard::TypeSymbol* withinType);
		static void PopFrame();

		static ArgumentsSpan& CreateArgumentsSpan(std::vector<shard::ArgumentSyntax*> arguments, std::vector<shard::ParameterSymbol*> parameters, bool isStatic, shard::ObjectInstance* instance);
		static ArgumentsSpan& CurrentContext();
		static void PushContext(ArgumentsSpan& context);
		static void PopContext();

		static void TerminateCallStack();
		static void Execute(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel);
		static void RaiseException(ObjectInstance* exceptionReg);

		static ObjectInstance* ExecuteMethod(const shard::MethodSymbol* method, const shard::TypeSymbol* withinType, ArgumentsSpan& argumentsContext);
		static ObjectInstance* ExecuteBlock(const shard::StatementsBlockSyntax* block);

		static ObjectInstance* ExecuteStatement(const shard::StatementSyntax* statement);
		static ObjectInstance* ExecuteExpressionStatement(const shard::ExpressionStatementSyntax* statement);
		static ObjectInstance* ExecuteVariableStatement(const shard::VariableStatementSyntax* statement);
		static ObjectInstance* ExecuteReturnStatement(const shard::ReturnStatementSyntax* statement);
		static ObjectInstance* ExecuteThrowStatement(const shard::ThrowStatementSyntax* statement);
		static ObjectInstance* ExecuteBreakStatement(const shard::BreakStatementSyntax* statement);
		static ObjectInstance* ExecuteContinueStatement(const shard::ContinueStatementSyntax* statement);

		static ObjectInstance* ExecuteIfStatement(const shard::IfStatementSyntax* statement);
		static ObjectInstance* ExecuteUnlessStatement(const shard::UnlessStatementSyntax* statement);
		static ObjectInstance* ExecuteElseStatement(const shard::ElseStatementSyntax* statement);

		static ObjectInstance* ExecuteForLoopStatement(const shard::ForStatementSyntax* statement);
		static ObjectInstance* ExecuteWhileLoopStatement(const shard::WhileStatementSyntax* statement);
		static ObjectInstance* ExecuteUntilLoopStatement(const shard::UntilStatementSyntax* statement);

		static ObjectInstance* EvaluateExpression(const shard::ExpressionSyntax* expression);
		static ObjectInstance* EvaluateLiteralExpression(const shard::LiteralExpressionSyntax* expression);
		static ObjectInstance* EvaluateObjectExpression(const shard::ObjectExpressionSyntax* expression);
		static ObjectInstance* EvaluateBinaryExpression(const shard::BinaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateAssignExpression(const shard::BinaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateUnaryExpression(const shard::UnaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateCollectionExpression(const shard::CollectionExpressionSyntax* expression);
		static ObjectInstance* EvaluateLambdaExpression(const shard::LambdaExpressionSyntax* expression);
		static ObjectInstance* EvaluateTernaryExpression(const shard::TernaryExpressionSyntax* expression);
		//static ExpressionSyntax* ChooseTernaryExpression(const shard::TernaryExpressionSyntax* expression);

		static ObjectInstance* EvaluateMemberAccessExpression(const shard::MemberAccessExpressionSyntax* expression, shard::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateInvokationExpression(const shard::InvokationExpressionSyntax* expression, shard::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateIndexatorExpression(const shard::IndexatorExpressionSyntax* expression, shard::ObjectInstance* prevInstance);

	private:
		static ObjectInstance* EvaluateArgument(const shard::ArgumentSyntax* argument);
		static void ExecuteInstanceSetter(ObjectInstance* instance, const shard::MemberAccessExpressionSyntax* access, ObjectInstance* value);
	};
}
*/
// --- End: shard/runtime/AbstractInterpreter.h ---

// --- Begin: shard/runtime/ConsoleHelper.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/ObjectInstance.h" (Merged)
#include <string>

namespace shard
{
	class SHARD_API ConsoleHelper
	{
	public:
		static void Write(ObjectInstance* instance);
		static void Write(bool data);
		static void Write(int64_t data);
		static void Write(double data);
		static void Write(wchar_t data);
		static void Write(const wchar_t* data);
		static void Write(std::wstring data);

		static void WriteLine(ObjectInstance* instance);
		static void WriteLine();
		static void WriteLine(bool data);
		static void WriteLine(int64_t data);
		static void WriteLine(double data);
		static void WriteLine(wchar_t data);
		static void WriteLine(const wchar_t* data);
		static void WriteLine(std::wstring data);
	};
}
// --- End: shard/runtime/ConsoleHelper.h ---

// --- Begin: shard/runtime/GarbageCollector.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/ObjectInstance.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)

#include <unordered_map>
#include <iterator>
#include <cstdint>

namespace shard
{
    class VirtualMachine;

    template<typename MapType>
    class SHARD_API ValueIterator
    {
    private:
        using MapIterator = typename MapType::iterator;
        MapIterator it;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename MapType::mapped_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        ValueIterator(MapIterator iterator) : it(iterator) {}

        value_type& operator*() { return it->second; }
        value_type* operator->() { return &it->second; }

        const value_type& operator*() const { return it->second; }
        const value_type* operator->() const { return &it->second; }

        ValueIterator& operator++() { ++it; return *this; }
        ValueIterator operator++(int) { ValueIterator temp = *this; ++it; return temp; }

        bool operator==(const ValueIterator& other) const { return it == other.it; }
        bool operator!=(const ValueIterator& other) const { return it != other.it; }
    };

	class SHARD_API InstancesHeap
	{
    private:
        std::unordered_map<uint64_t, shard::ObjectInstance*> IdMap;
        std::unordered_map<void*, shard::ObjectInstance*> PtrMap;

    public:
        using iterator = ValueIterator<decltype(IdMap)>;
        using const_iterator = ValueIterator<const decltype(IdMap)>;

        inline void add(ObjectInstance* instance)
        {
            IdMap[instance->Id] = instance;
            PtrMap[(void*)instance->Ptr] = instance;
        }

        inline iterator begin() { return iterator(IdMap.begin()); }
        inline iterator end() { return iterator(IdMap.end()); }

        inline auto pairs_begin() { return IdMap.begin(); }
        inline auto pairs_end() { return IdMap.end(); }

        inline shard::ObjectInstance* at(uint64_t id) { return IdMap.at(id); }
        inline shard::ObjectInstance* at(void* ptr) { return PtrMap.at(ptr); }

        inline void erase(shard::ObjectInstance* instance)
        {
            IdMap.erase(instance->Id);
            PtrMap.erase((void*)instance->Ptr);
        }

        inline void clear()
        {
            IdMap.clear();
            PtrMap.clear();
        }

        inline size_t size()
        {
            return IdMap.size();
        }
	};

	class SHARD_API GarbageCollector
	{
		inline static uint64_t objectsCounter = 0;
        inline static std::unordered_map<TypeSymbol*, ObjectInstance*> nullInstancesMap;
        inline static std::unordered_map<FieldSymbol*, ObjectInstance*> staticFields;

    public:
		inline static InstancesHeap Heap;
        static ObjectInstance* NullInstance;

        static ObjectInstance* GetStaticField(const VirtualMachine* host, FieldSymbol* field);
        static void SetStaticField(const VirtualMachine* host, FieldSymbol* field, ObjectInstance* instance);

		static ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo);
        static ObjectInstance* CopyInstance(const TypeSymbol* objectInfo, void* ptr);
        static ObjectInstance* CopyInstance(ObjectInstance* instance);

        static void CollectInstance(ObjectInstance* instance);
        static void DestroyInstance(ObjectInstance* instance);
        static void TerminateInstance(ObjectInstance* instance);
		static void Terminate();
	};
}
// --- End: shard/runtime/GarbageCollector.h ---

// --- Begin: shard/runtime/PrimitiveMathModule.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/runtime/ObjectInstance.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API PrimitiveMathModule
	{
	public:
		static shard::ObjectInstance* EvaluateBinaryOperator(shard::ObjectInstance* leftInstance, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(int64_t leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(bool leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(std::wstring& leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);

		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, int64_t data, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, bool data, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, std::wstring& data, TokenType opToken, bool rightDetermined);
	};
}
// --- End: shard/runtime/PrimitiveMathModule.h ---

// --- Begin: shard/runtime/ProgramDisassembler.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/compilation/ProgramVirtualImage.h" (Merged)

#include <ostream>

namespace shard
{
	class SHARD_API ProgramDisassembler
	{
	public:
		void Disassemble(std::wostream& out, ProgramVirtualImage& program);
	};
}
// --- End: shard/runtime/ProgramDisassembler.h ---

// --- Begin: shard/runtime/VirtualMachine.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/compilation/ProgramVirtualImage.h" (Merged)
// #include "shard/compilation/ByteCodeDecoder.h" (Merged)
// #include "shard/compilation/OperationCode.h" (Merged)

// #include "shard/runtime/CallStackFrame.h" (Merged)
// #include "shard/runtime/ObjectInstance.h" (Merged)

// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)

#include <stack>
#include <atomic>
#include <initializer_list>

namespace shard
{
	class SHARD_API VirtualMachine
	{
		ProgramVirtualImage& Program;
		std::stack<CallStackFrame*> CallStack;
		std::atomic<bool> AbortFlag;

		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		void InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame);
		ObjectInstance* InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor);

	public:
		VirtualMachine(ProgramVirtualImage& program);

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		void PopFrame();

		void InvokeMethod(MethodSymbol* method);
		void InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const;
		void RaiseException(ObjectInstance* exceptionReg);

		void Run();
		void TerminateCallStack();
	};
}
// --- End: shard/runtime/VirtualMachine.h ---

// --- Begin: shard/runtime/framework/FrameworkModule.h ---
// #include "shard/ShardScriptAPI.h" (Merged)
// #include "shard/parsing/lexical/SourceProvider.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/AccessorSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)

#include <string>

namespace shard
{
	class SHARD_API FrameworkModule
	{
	public:
		virtual shard::SourceProvider* GetSource() = 0;
		virtual bool BindConstructor(shard::ConstructorSymbol* symbol) = 0;
		virtual bool BindMethod(shard::MethodSymbol* symbol) = 0;
		virtual bool BindAccessor(shard::AccessorSymbol* symbol) = 0;
	};
}
// --- End: shard/runtime/framework/FrameworkModule.h ---

// --- Begin: shard/runtime/framework/FrameworkLoader.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/runtime/framework/FrameworkModule.h" (Merged)
// #include "shard/parsing/semantic/SemanticModel.h" (Merged)
// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)

#include <Windows.h>
#include <vector>

namespace shard
{
	class SHARD_API FrameworkLoader
	{
	private:
		static std::vector<HMODULE> LoadedLibraries;
		static std::vector<shard::FrameworkModule*> Modules;

	public:
		static void AddLib(const std::wstring& path);
		static void AddModule(shard::FrameworkModule* pModule);
		static void Destroy();

		static void Load(shard::SemanticModel& semanticModel, shard::DiagnosticsContext& diagnostics);
	};
}
// --- End: shard/runtime/framework/FrameworkLoader.h ---

// --- Begin: shard/syntax/symbols/ClassSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API ClassSymbol : public TypeSymbol
	{
	public:
		inline ClassSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::ClassDeclaration)
		{
			IsReferenceType = true;
		}

		inline ClassSymbol(const ClassSymbol& other) = delete;

		inline virtual ~ClassSymbol() override
		{

		}
	};
}
// --- End: shard/syntax/symbols/ClassSymbol.h ---

// --- Begin: shard/syntax/symbols/StructSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

// #include "shard/syntax/SyntaxKind.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API StructSymbol : public TypeSymbol
	{
	public:
		//std::vector<MethodSymbol*> Constructors;

		inline StructSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::StructDeclaration)
		{
			//MemoryBytesSize += 0;
			IsValueType = true;
		}

		inline StructSymbol(const StructSymbol& other) = delete;

		inline virtual ~StructSymbol() override
		{
			/*
			for (MethodSymbol* ctor : Constructors)
				delete ctor;
			*/
		}
	};
}
// --- End: shard/syntax/symbols/StructSymbol.h ---

// --- Begin: shard/syntax/SymbolFactory.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/parsing/analysis/DiagnosticsContext.h" (Merged)
// #include "shard/parsing/semantic/SymbolTable.h" (Merged)

// #include "shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h" (Merged)
// #include "shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h" (Merged)

// #include "shard/syntax/nodes/Types/ArrayTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/GenericTypeSyntax.h" (Merged)
// #include "shard/syntax/nodes/Types/DelegateTypeSyntax.h" (Merged)

// #include "shard/syntax/symbols/NamespaceSymbol.h" (Merged)
// #include "shard/syntax/symbols/ClassSymbol.h" (Merged)
// #include "shard/syntax/symbols/StructSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)
// #include "shard/syntax/symbols/ConstructorSymbol.h" (Merged)
// #include "shard/syntax/symbols/TypeParameterSymbol.h" (Merged)
// #include "shard/syntax/symbols/VariableSymbol.h" (Merged)
// #include "shard/syntax/symbols/ParameterSymbol.h" (Merged)
// #include "shard/syntax/symbols/IndexatorSymbol.h" (Merged)
// #include "shard/syntax/symbols/AccessorSymbol.h" (Merged)
// #include "shard/syntax/symbols/GenericTypeSymbol.h" (Merged)

// #include "shard/syntax/symbols/DelegateTypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/ArrayTypeSymbol.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)
// #include "shard/syntax/SymbolAccesibility.h" (Merged)

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API SymbolFactory
	{
	public:
		static void SetAccesibility(shard::SyntaxSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::TypeSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::FieldSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::PropertySymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::MethodSymbol* node, std::vector<shard::SyntaxToken> modifiers);

		static shard::StructSymbol* Struct(shard::StructDeclarationSyntax* node);
		static shard::ClassSymbol* Class(shard::ClassDeclarationSyntax* node);
		static shard::NamespaceSymbol* Namespace(shard::NamespaceDeclarationSyntax* node);
		static shard::NamespaceSymbol* Namespace(const std::wstring& name);

		static shard::FieldSymbol* Field(shard::FieldDeclarationSyntax* node);
		static shard::FieldSymbol* Field(const std::wstring& name, shard::TypeSymbol* type, bool isStatic = false);

		static shard::PropertySymbol* Property(shard::PropertyDeclarationSyntax* node);
		static shard::PropertySymbol* Property(const std::wstring& name, shard::TypeSymbol* returnType, bool isStatic = false);

		static shard::MethodSymbol* Method(shard::MethodDeclarationSyntax* node);
		static shard::MethodSymbol* Method(const std::wstring& name, shard::TypeSymbol* returnType, bool isStatic = false);

		static shard::ConstructorSymbol* Constructor(shard::ConstructorDeclarationSyntax* node);
		static shard::ConstructorSymbol* Constructor(const std::wstring& name);

		static shard::AccessorSymbol* Accessor(shard::AccessorDeclarationSyntax* node, shard::PropertySymbol* propertySymbol, bool setProperty = true);
		static shard::AccessorSymbol* Accessor(const std::wstring& name, shard::PropertySymbol* property, bool isGetter);
		static shard::AccessorSymbol* Getter(const std::wstring& propertyName, shard::PropertySymbol* property);
		static shard::AccessorSymbol* Setter(const std::wstring& propertyName, shard::PropertySymbol* property);

		static shard::IndexatorSymbol* Indexator(shard::IndexatorDeclarationSyntax* node);
		static shard::IndexatorSymbol* Indexator(const std::wstring& name, shard::TypeSymbol* returnType);
		static shard::IndexatorSymbol* Indexator(const std::wstring& name, shard::TypeSymbol* returnType, std::vector<shard::ParameterSymbol*> parameters);

		static shard::ParameterSymbol* Parameter(const std::wstring& name);
		static shard::ParameterSymbol* Parameter(const std::wstring& name, shard::TypeSymbol* type);
		static shard::ParameterSymbol* Parameter(const std::wstring& name, shard::TypeSymbol* type, bool isOptional);

		static shard::VariableSymbol* Variable(const std::wstring& name, shard::TypeSymbol* type);
		static shard::VariableSymbol* Variable(const std::wstring& name, shard::TypeSymbol* type, bool isConst);

		static shard::TypeParameterSymbol* TypeParameter(const std::wstring& name);

		static shard::DelegateTypeSymbol* Delegate(shard::DelegateDeclarationSyntax* node);
		static shard::DelegateTypeSymbol* Delegate(shard::DelegateTypeSyntax* node);
		static shard::DelegateTypeSymbol* Delegate(shard::MethodSymbol* method);
		static shard::DelegateTypeSymbol* Delegate(const std::wstring& name, shard::TypeSymbol* returnType, std::vector<shard::ParameterSymbol*> parameters);

		static shard::ArrayTypeSymbol* Array(shard::ArrayTypeSyntax* node);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType, size_t size);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType, size_t size, int rank);

		static shard::GenericTypeSymbol* GenericType(shard::TypeSymbol* underlayingType);
		static shard::GenericTypeSymbol* GenericType(shard::TypeSymbol* underlayingType, std::unordered_map<std::wstring, shard::TypeSymbol*> typeArguments);

		static std::wstring FormatFullName(shard::SyntaxSymbol* symbol);
		static std::wstring FormatFullName(shard::SyntaxSymbol* symbol, shard::SyntaxSymbol* parent);
		static std::wstring FormatMethodSignature(shard::MethodSymbol* method);
		static std::wstring FormatTypeName(shard::TypeSymbol* type);

		static shard::MethodSymbol* CreateAnonymousMethod(const std::wstring& name, shard::TypeSymbol* returnType);
		static shard::MethodSymbol* CreateLambdaMethod(shard::StatementsBlockSyntax* body);
	};
}
// --- End: shard/syntax/SymbolFactory.h ---

// --- Begin: shard/syntax/SyntaxFacts.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/TokenType.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)

SHARD_API bool IsPunctuation(shard::TokenType type);

SHARD_API int GetOperatorPrecendence(shard::TokenType type);
SHARD_API bool IsOperator(shard::TokenType type);

SHARD_API bool IsBinaryOperator(shard::TokenType type);
SHARD_API bool IsBinaryArithmeticOperator(shard::TokenType type);
SHARD_API bool IsBinaryBooleanOperator(shard::TokenType type);
SHARD_API bool IsBinaryBitOperator(shard::TokenType type);

SHARD_API bool IsUnaryOperator(shard::TokenType type);
SHARD_API bool IsRightUnaryOperator(shard::TokenType type);
SHARD_API bool IsRightUnaryArithmeticOperator(shard::TokenType type);
SHARD_API bool IsRightUnaryBooleanOperator(shard::TokenType type);

SHARD_API bool IsLeftUnaryOperator(shard::TokenType type);
SHARD_API bool IsLeftUnaryArithmeticOperator(shard::TokenType type);
SHARD_API bool IsLeftUnaryBooleanOperator(shard::TokenType type);

SHARD_API bool IsModifier(shard::TokenType type);
SHARD_API bool IsTypeKeyword(shard::TokenType type);
SHARD_API bool IsMemberKeyword(shard::TokenType type);
SHARD_API bool IsMemberDeclaration(shard::TokenType currentType, shard::TokenType peekType);

SHARD_API bool IsPredefinedType(shard::TokenType type);
SHARD_API bool IsType(shard::TokenType type, shard::TokenType peekType);

SHARD_API bool IsKeyword(shard::TokenType type);
SHARD_API bool IsLoopKeyword(shard::TokenType type);
SHARD_API bool IsConditionalKeyword(shard::TokenType type);
SHARD_API bool IsFunctionalKeyword(shard::TokenType type);

SHARD_API bool IsLinkedExpressionNode(shard::SyntaxKind kind);

//SHARD_API bool IsKeywordHasExpression(shard::TokenType type);
//SHARD_API bool IsMethodInvokationExpression(shard::TokenType current, shard::TokenType peek);
// --- End: shard/syntax/SyntaxFacts.h ---

// --- Begin: shard/syntax/SyntaxHelpers.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxToken.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/symbols/FieldSymbol.h" (Merged)
// #include "shard/syntax/symbols/MethodSymbol.h" (Merged)

#include <vector>

// --- End: shard/syntax/SyntaxHelpers.h ---

// --- Begin: shard/syntax/symbols/LeftDenotationSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/symbols/TypeSymbol.h" (Merged)
// #include "shard/syntax/SyntaxKind.h" (Merged)
// #include "shard/syntax/SyntaxSymbol.h" (Merged)

namespace shard
{
    class SHARD_API LeftDenotationSymbol : public SyntaxSymbol
    {
    public:
        const TypeSymbol* ExpectedType;

        inline LeftDenotationSymbol(const TypeSymbol* expectedType)
            : SyntaxSymbol(L"", SyntaxKind::Argument), ExpectedType(expectedType) {}

        inline LeftDenotationSymbol(const LeftDenotationSymbol& other) = delete;

        inline ~LeftDenotationSymbol()
        {

        }
    };
}
// --- End: shard/syntax/symbols/LeftDenotationSymbol.h ---

// --- Begin: shard/syntax/symbols/LiteralSymbol.h ---
// #include "shard/ShardScriptAPI.h" (Merged)

// #include "shard/syntax/SyntaxSymbol.h" (Merged)

namespace shard
{
	class SHARD_API LiteralSymbol : public SyntaxSymbol
	{
	public:
		union
		{
			bool AsBooleanValue = false;
			int64_t AsIntegerValue;
			double AsDoubleValue;
		};

		const TokenType LiteralType;

		LiteralSymbol(const TokenType type)
			: SyntaxSymbol(L"Literal", SyntaxKind::LiteralExpression), LiteralType(type) { }
	};
}
// --- End: shard/syntax/symbols/LiteralSymbol.h ---

#endif // SHARDSCRIPT_SINGLE_HEADER_H