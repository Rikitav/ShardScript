#pragma once
#ifndef SHARDSCRIPT_SINGLE_HEADER_H
#define SHARDSCRIPT_SINGLE_HEADER_H

// ============================================================================
// ShardScript Unified Header
// ============================================================================
// This file includes all public ShardScript headers in dependency-safe order.
// Include this single header to use the entire ShardScript API.
//
// Note: This is a manually maintained umbrella header. If you add a new public
// header, please append it here in an appropriate location (preferably near
// related headers) or rerun the dependency sort.
// ============================================================================

// --- Core / Platform ---
#include <shard/ShardScriptAPI.hpp>

// --- Syntax / Foundation ---
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SymbolBuilder.hpp>

// --- Semantic / Parsing Infrastructure ---
#include <shard/parsing/analysis/TextLocation.hpp>
#include <shard/parsing/analysis/DiagnosticSeverity.hpp>
#include <shard/parsing/analysis/Diagnostic.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>
#include <shard/parsing/semantic/TypeInfo.hpp>
#include <shard/parsing/semantic/SymbolInfo.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/LayoutGenerator.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>

// --- Syntax / Symbols ---
#include <shard/syntax/symbols/MemberSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/LeftDenotationSymbol.hpp>
#include <shard/syntax/symbols/LiteralSymbol.hpp>

// --- Syntax / Nodes ---
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/BodyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>

// --- Syntax / Directives ---
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

// --- Syntax / Compilation Unit ---
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>

// --- Syntax / Statements ---
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

// --- Syntax / Loops ---
#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>

// --- Syntax / Member Declarations ---
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>

// --- Syntax / Expressions ---
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>

// --- Syntax / Types ---
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>

// --- Attributes ---
#include <shard/syntax/nodes/AttributeSyntax.hpp>

// --- Syntax / Utilities ---
#include <shard/syntax/SymbolFactory.hpp>
#include <shard/syntax/SyntaxFacts.hpp>
#include <shard/syntax/SyntaxHelpers.hpp>

// --- Compilation ---
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/AbstractEmiter.hpp>

// --- Runtime ---
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/ConsoleHelper.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ProgramDisassembler.hpp>

// --- Application / Context ---
#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/ShardScriptLIB.hpp>

// --- Lexical / Reading ---
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/lexical/reading/SourceTextProvider.hpp>
#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/LexicalBuffer.hpp>
#include <shard/parsing/lexical/reading/FileReader.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>

// --- Semantic Visitors ---
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/semantic/visiting/DeclarationCollector.hpp>
#include <shard/parsing/semantic/visiting/ExpressionBinder.hpp>
#include <shard/parsing/semantic/visiting/TypeBinder.hpp>

// --- Visitor ---
#include <shard/SyntaxVisitor.hpp>

#endif // SHARDSCRIPT_SINGLE_HEADER_H
