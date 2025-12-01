#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>

SHARD_API bool IsPunctuation(shard::syntax::TokenType type);

SHARD_API int GetOperatorPrecendence(shard::syntax::TokenType type);
SHARD_API bool IsOperator(shard::syntax::TokenType type);

SHARD_API bool IsBinaryOperator(shard::syntax::TokenType type);
SHARD_API bool IsBinaryArithmeticOperator(shard::syntax::TokenType type);
SHARD_API bool IsBinaryBooleanOperator(shard::syntax::TokenType type);
SHARD_API bool IsBinaryBitOperator(shard::syntax::TokenType type);

SHARD_API bool IsUnaryOperator(shard::syntax::TokenType type);
SHARD_API bool IsRightUnaryOperator(shard::syntax::TokenType type);
SHARD_API bool IsRightUnaryArithmeticOperator(shard::syntax::TokenType type);
SHARD_API bool IsRightUnaryBooleanOperator(shard::syntax::TokenType type);

SHARD_API bool IsLeftUnaryOperator(shard::syntax::TokenType type);
SHARD_API bool IsLeftUnaryArithmeticOperator(shard::syntax::TokenType type);
SHARD_API bool IsLeftUnaryBooleanOperator(shard::syntax::TokenType type);

SHARD_API bool IsModifier(shard::syntax::TokenType type);
SHARD_API bool IsMemberKeyword(shard::syntax::TokenType type);
SHARD_API bool IsMemberDeclaration(shard::syntax::TokenType currentType, shard::syntax::TokenType peekType);

SHARD_API bool IsPredefinedType(shard::syntax::TokenType type);
SHARD_API bool IsType(shard::syntax::TokenType type, shard::syntax::TokenType peekType);

SHARD_API bool IsKeyword(shard::syntax::TokenType type);
SHARD_API bool IsLoopKeyword(shard::syntax::TokenType type);
SHARD_API bool IsConditionalKeyword(shard::syntax::TokenType type);
SHARD_API bool IsFunctionalKeyword(shard::syntax::TokenType type);

SHARD_API bool IsLinkedExpressionNode(shard::syntax::SyntaxKind kind);

//SHARD_API bool IsKeywordHasExpression(shard::syntax::TokenType type);
//SHARD_API bool IsMethodInvokationExpression(shard::syntax::TokenType current, shard::syntax::TokenType peek);
