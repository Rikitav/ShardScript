#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>

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
