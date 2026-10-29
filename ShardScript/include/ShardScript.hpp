#pragma once
#ifndef _SHARDSCRIPT_
#define _SHARDSCRIPT_

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

#ifdef _WINDOWS_
	#error Never include Windows.h before ShardScript.hpp
#else

// --- GMT (general memory toolkit) ---
#include <gmt/Alignment.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Allocator.hpp>
#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>
#include <gmt/Arena.hpp>
#include <gmt/FlatMap.hpp>
#include <gmt/InlineVector.hpp>
#include <gmt/Interner.hpp>
#include <gmt/Strings.hpp>
#include <gmt/Concat.hpp>
#include <gmt/SharedLibrary.hpp>

// --- Core / Platform ---
#include <shard/Definitions.hpp>
#include <shard/LibraryMetadata.hpp>

// --- Lexical ---
#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/lexical/StringStreamReader.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/lexical/LexicalBuffer.hpp>
#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>

// --- Parsing ---
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SemanticVersion.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>

#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>
#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>

#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/AttributesListSyntax.hpp>
#include <shard/parsing/nodes/lists/BaseTypesListSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/types/QualifiedNameTypeSyntax.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/parsing/SourceParser.hpp>

// --- Semantic ---
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/StandardTypes.hpp>

#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericMethodSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>

#include <shard/semantic/SymbolInterner.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticVisitor.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/MethodEffectSummary.hpp>
#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/TypeBinder.hpp>
#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/ControlFlowAnalyzer.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>

// --- Pragmatic ---
#include <shard/pragmatic/PlacementVerdict.hpp>
#include <shard/pragmatic/EscapeChannels.hpp>
#include <shard/pragmatic/EscapeSummary.hpp>
#include <shard/pragmatic/EscapeAnalyzer.hpp>
#include <shard/pragmatic/TypeShape.hpp>
#include <shard/pragmatic/TypeShapeCache.hpp>
#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/LayoutGenerator.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/PragmaticAnalyzer.hpp>

// --- Compilation ---
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/ByteCodeEmitter.hpp>
#include <shard/compilation/ProgramDisassembler.hpp>

// --- Runtime ---
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMath.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/NativeWrapper.hpp>
#include <shard/runtime/VirtualMachine.hpp>

// --- Driving ---
#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>

#endif // _WINDOWS_
#endif // _SHARDSCRIPT_
