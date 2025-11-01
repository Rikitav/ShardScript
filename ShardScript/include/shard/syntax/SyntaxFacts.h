#pragma once
#include <shard/syntax/TokenType.h>

int GetOperatorPrecendence(shard::syntax::TokenType type);
bool IsBinaryOperator(shard::syntax::TokenType type);
bool IsBinaryArithmeticOperator(shard::syntax::TokenType type);
bool IsBinaryBooleanOperator(shard::syntax::TokenType type);
bool IsUnaryOperator(shard::syntax::TokenType type);
bool IsUnaryArithmeticOperator(shard::syntax::TokenType type);
bool IsUnaryBooleanOperator(shard::syntax::TokenType type);
bool IsOperator(shard::syntax::TokenType type);
bool IsMemberKeyword(shard::syntax::TokenType type);
bool IsPredefinedType(shard::syntax::TokenType type);
bool IsType(shard::syntax::TokenType type, shard::syntax::TokenType peekType);
bool IsModifier(shard::syntax::TokenType type);
bool IsMemberDeclaration(shard::syntax::TokenType currentType, shard::syntax::TokenType peekType);
bool IsLoopKeyword(shard::syntax::TokenType type);
bool IsConditionalKeyword(shard::syntax::TokenType type);
bool IsFunctionalKeyword(shard::syntax::TokenType type);
bool IsKeyword(shard::syntax::TokenType type);
bool IsKeywordHasExpression(shard::syntax::TokenType type);
bool IsMethodInvokationExpression(shard::syntax::TokenType current, shard::syntax::TokenType peek);