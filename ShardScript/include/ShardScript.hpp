#pragma once
#ifndef SHARDSCRIPT_SINGLE_HEADER_H
#define SHARDSCRIPT_SINGLE_HEADER_H

/*
*                                  .:::::::.
*                              .:::-+-:::-+-:::.
*                          .::::-+-:::@@@:::-=-::::.
*                      .:::-+-:::.@@@@@@@@@@@:::-+-:::.
*                  .:::-+-:::.@@@@@@@@@@@@@@@@@@.:::-+-:::.
*              .:::-+-:::.@@@@@@@@@@@@@@@@@@@@@@@@@@.:::-+-:::.
*          .:::-+-:::.@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@.:::-+-:::.
*      .:::-++-::.@@@@@@@@@@@@@@@@@.:::::::-@@@@@@@@@@@@@@@@.:::-+-:::.     
*    ...##=-::.@@@@@@@@@@@@@@@@.:::-++++++--:::.@@@@@@@@@@@@@@@@.:-=##=..
*    .=-:..@@@@@@@@@@@@@@@@.:::-+++++++++++++++-:::.@@@@@@@@@@@@@@@..:-=.    
*    .===.@@@@@@@@@@@@@@@..:+++++++++++++++++++++++-:::.@@@@@@@@@@@@.===.    ============================================================================
*    .===.@@@@@@@@@@@@@@@@@.::::-+++++++++++++++++++++*====@@@@@@@@@.===.    ShardScript Unified Header
*    .===:.@@@@@@@@@@@@@@@@@@@@@.:::-+++++++++++++*###%.....@@@@@@@@.===.    ============================================================================
*    .====:...@@@@@@@@@@@@@@@@@@@@@@.:::-+++++*###%....:===.@@@@@@@@.===.    This file includes all public ShardScript headers in dependency-safe order.
*    .=======:....@@@@@@@@@@@@@@@@@@@@@@.:::-=%....:=======.@@@@@@@@.===.    Include this single header to use the entire ShardScript API.
*    .===========::....@@@@@@@@@@@@@@@@@@@@@@....:=========:..@@@@@@.===.    Note: This is a manually maintained umbrella header. If you add a new public
*    .===:....:=======:....@@@@@@@@@@@@@@@@@@@@@@....:=======:......:===.    header, please append it here in an appropriate location (preferably near
*    .===.@@@@@@..:=======:....@@@@@@@@@@@@@@@@@@@@@@....:==============.    related headers) or rerun the dependency sort.
*    .===.@@@@@@@@.===========:....@@@@@@@@@@@@@@@@@@@@@@.....:=========.    ============================================================================
*    .===.@@@@@@@@.===============:.....@@@@@@@@@@@@@@@@@@@@@@....:=====.
*    .===.@@@@@@@@.===================+.....@@@@@@@@@@@@@@@@@@@@@@..:===.
*    .===.@@@@@@@@...:================+:===:....@@@@@@@@@@@@@@@@@@@@.===.
*    .===.@@@@@@@@@@@.....:===========+:=======:....@@@@@@@@@@@@@@@@.===.
*    .===.@@@@@@@@@@@@@@@@....:=======+:========:....@@@@@@@@@@@@@@@.===.
*    ..:=:..@@@@@@@@@@@@@@@@@@....:===+:====:....@@@@@@@@@@@@@@@@@..:=:..
*      .........@@@@@@@@@@@@@@@@@@...........@@@@@@@@@@@@@@@@@.........
*          .........@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@.........
*              ..........@@@@@@@@@@@@@@@@@@@@@@@@@@@@@.........
*                  ..........@@@@@@@@@@@@@@@@@@@@..........
*                      ..........:@@@@@@@@@@@..........
*                          ...........@@@..........
*                              ...............                          
*                                  ........                                  
*/

// --- Core / Platform ---
#include <shard/ShardScriptAPI.hpp>

// --- Syntax / Foundation ---
#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

// --- Compilation Analysis ---
#include <shard/analysis/TextLocation.hpp>
#include <shard/analysis/DiagnosticSeverity.hpp>
#include <shard/analysis/Diagnostic.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>

// --- Semantic / Parsing Infrastructure ---
#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/TypeInfo.hpp>
#include <shard/semantic/SymbolInfo.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/PrimitiveOperators.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>

// --- Syntax / Symbols ---
#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>
#include <shard/semantic/symbols/CompilationUnit.hpp>

// --- Syntax / Nodes ---
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/BodyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>

// --- Syntax / Directives ---
#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

// --- Syntax / Compilation Unit ---
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>

// --- Syntax / Statements ---
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

// --- Syntax / Loops ---
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>

// --- Syntax / Member Declarations ---
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>

// --- Syntax / Expressions ---
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>

// --- Syntax / Types ---
#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>

// --- Attributes ---
#include <shard/parsing/nodes/AttributeSyntax.hpp>

// --- Syntax / Utilities ---
#include <shard/semantic/SymbolFactory.hpp>

// --- Lexical / Reading ---
#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/LexicalBuffer.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/lexical/StringStreamReader.hpp>

// --- Parsing ---
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/parsing/SourceParser.hpp>

// --- Semantic Visitors ---
#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/TypeBinder.hpp>

// --- Compilation ---
#include <shard/compilation/LayoutGenerator.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/AbstractEmiter.hpp>
#include <shard/compilation/ProgramDisassembler.hpp>

// --- Runtime ---
#include <shard/runtime/TypeShape.hpp>
#include <shard/runtime/TypeShapeCache.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/ConsoleHelper.hpp>
#include <shard/runtime/MethodCallState.hpp>

// --- Application / Context ---
#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/ShardScriptLIB.hpp>

// --- Public C/C++ API ---
#include <shard/ShardScriptExtern.hpp>

#endif // SHARDSCRIPT_SINGLE_HEADER_H
