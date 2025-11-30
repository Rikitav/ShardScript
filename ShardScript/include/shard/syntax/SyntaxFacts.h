#pragma once
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>

bool IsPunctuation(shard::syntax::TokenType type);

int GetOperatorPrecendence(shard::syntax::TokenType type);
bool IsOperator(shard::syntax::TokenType type);

bool IsBinaryOperator(shard::syntax::TokenType type);
bool IsBinaryArithmeticOperator(shard::syntax::TokenType type);
bool IsBinaryBooleanOperator(shard::syntax::TokenType type);
bool IsBinaryBitOperator(shard::syntax::TokenType type);

bool IsUnaryOperator(shard::syntax::TokenType type);
bool IsRightUnaryOperator(shard::syntax::TokenType type);
bool IsRightUnaryArithmeticOperator(shard::syntax::TokenType type);
bool IsRightUnaryBooleanOperator(shard::syntax::TokenType type);

bool IsLeftUnaryOperator(shard::syntax::TokenType type);
bool IsLeftUnaryArithmeticOperator(shard::syntax::TokenType type);
bool IsLeftUnaryBooleanOperator(shard::syntax::TokenType type);

bool IsModifier(shard::syntax::TokenType type);
bool IsMemberKeyword(shard::syntax::TokenType type);
bool IsMemberDeclaration(shard::syntax::TokenType currentType, shard::syntax::TokenType peekType);

bool IsPredefinedType(shard::syntax::TokenType type);
bool IsType(shard::syntax::TokenType type, shard::syntax::TokenType peekType);

bool IsKeyword(shard::syntax::TokenType type);
bool IsLoopKeyword(shard::syntax::TokenType type);
bool IsConditionalKeyword(shard::syntax::TokenType type);
bool IsFunctionalKeyword(shard::syntax::TokenType type);

bool IsLinkedExpressionNode(shard::syntax::SyntaxKind kind);

//bool IsKeywordHasExpression(shard::syntax::TokenType type);
//bool IsMethodInvokationExpression(shard::syntax::TokenType current, shard::syntax::TokenType peek);