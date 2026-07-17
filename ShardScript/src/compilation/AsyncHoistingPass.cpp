#include <shard/compilation/AsyncHoistingPass.hpp>

#include <format>
#include <memory>
#include <vector>

#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>

#define HR_INVALID HoistResult{ .Expression = nullptr, .Prefix = {}, .Supported = false };

using namespace shard;

namespace
{
    struct HoistResult
    {
        std::unique_ptr<ExpressionSyntax> Expression = nullptr;
        std::vector<std::unique_ptr<StatementSyntax>> Prefix = {};
        bool Supported = true;
    };

    static bool IsShortCircuitOrConditionalOperator(TokenType op)
    {
        return op == TokenType::AndOperator
            || op == TokenType::OrOperator
            || op == TokenType::NullCoalescingOperator;
    }

    static std::unique_ptr<MemberAccessExpressionSyntax> CreateVariableReference(VariableSymbol* variable, SyntaxNode* parent)
    {
        SyntaxToken token(TokenType::Identifier, variable->Name, shard::TextLocation());

        auto node = std::make_unique<MemberAccessExpressionSyntax>(token, nullptr, parent);
        node->ToVariable = variable;
        return node;
    }

    static void MovePrefix(std::vector<std::unique_ptr<StatementSyntax>>& dst, std::vector<std::unique_ptr<StatementSyntax>>& src)
    {
        if (src.empty())
            return;

        dst.insert(dst.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
        src.clear();
    }

    static HoistResult HoistExpression(std::unique_ptr<ExpressionSyntax> expr, bool allowTopLevelAwait, bool conditional, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics)
    {
        if (expr == nullptr)
            return { nullptr, {}, true };

        if (expr->Kind == SyntaxKind::AwaitExpression)
        {
            AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(expr.get());

            // Hoist any awaits inside the awaitable expression first.
            HoistResult awaitable = HoistExpression(std::move(awaitExpr->Expression), false, false, method, awaitExpr, model, diagnostics);
            if (!awaitable.Supported)
                return HR_INVALID;

            awaitExpr->Expression = std::move(awaitable.Expression);
            if (allowTopLevelAwait)
            {
                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, awaitable.Prefix);
                return result;
            }

            if (conditional)
            {
                diagnostics.ReportError(awaitExpr->AwaitKeywordToken, L"Await expression inside a conditional branch is not yet supported in async methods");
                return HR_INVALID;
            }

            MethodSymbol* getResult = awaitExpr->GetResultMethod;
            TypeSymbol* resultType = (getResult != nullptr && getResult->ReturnType != nullptr)
                ? const_cast<TypeSymbol*>(getResult->ReturnType) : SymbolTable::Primitives::Any;

            if (resultType == SymbolTable::Primitives::Void)
            {
                diagnostics.ReportError(awaitExpr->AwaitKeywordToken, L"A void await cannot be used as a sub-expression");
                return HR_INVALID;
            }

            static int condCounter = 0;
            condCounter = 0;

            std::wstring varName = std::format(L"<a>__condTmp{}", condCounter++);
            SyntaxToken nameToken(TokenType::Identifier, varName, awaitExpr->AwaitKeywordToken.Location);
            SyntaxToken assignToken(TokenType::DeclareAssignOperator, L":=", awaitExpr->AwaitKeywordToken.Location);

            std::unique_ptr<VariableStatementSyntax> varStmt = std::make_unique<VariableStatementSyntax>(nullptr, nameToken, assignToken, std::move(expr), parent);
            VariableSymbol* varSymbol = model.Table->BindSymbolEx<VariableSymbol>(varStmt.get(), std::make_unique<VariableSymbol>(varName, resultType));

            varSymbol->SlotIndex = static_cast<std::uint16_t>(method->GetEvalStackArgumentsCount() + method->AddVariableCount());
            varSymbol->Parent = method;
            varSymbol->FullName = method->FullName + L"." + varName;

            HoistResult result;
            result.Prefix.push_back(std::move(varStmt));
            MovePrefix(result.Prefix, awaitable.Prefix);

            result.Expression = CreateVariableReference(varSymbol, parent);
            return result;
        }

        switch (expr->Kind)
        {
            case SyntaxKind::BinaryExpression:
            {
                BinaryExpressionSyntax* node = static_cast<BinaryExpressionSyntax*>(expr.get());

                bool rightConditional = conditional;
                if (!conditional && IsShortCircuitOrConditionalOperator(node->OperatorToken.Type))
                    rightConditional = true;

                HoistResult left = HoistExpression(std::move(node->Left), false, conditional, method, node, model, diagnostics);
                if (!left.Supported)
                    return HR_INVALID;

                HoistResult right = HoistExpression(std::move(node->Right), false, rightConditional, method, node, model, diagnostics);
                if (!right.Supported)
                    return HR_INVALID;

                node->Left = std::move(left.Expression);
                node->Right = std::move(right.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, left.Prefix);
                MovePrefix(result.Prefix, right.Prefix);

                return result;
            }

            case SyntaxKind::UnaryExpression:
            {
                UnaryExpressionSyntax* node = static_cast<UnaryExpressionSyntax*>(expr.get());

                HoistResult inner = HoistExpression(std::move(node->Expression), false, conditional, method, node, model, diagnostics);
                if (!inner.Supported)
                    return HR_INVALID;

                node->Expression = std::move(inner.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, inner.Prefix);

                return result;
            }

            case SyntaxKind::TernaryExpression:
            {
                TernaryExpressionSyntax* node = static_cast<TernaryExpressionSyntax*>(expr.get());

                HoistResult cond = HoistExpression(std::move(node->Condition), false, conditional, method, node, model, diagnostics);
                if (!cond.Supported)
                    return HR_INVALID;

                HoistResult left = HoistExpression(std::move(node->Left), false, true, method, node, model, diagnostics);
                if (!left.Supported)
                    return HR_INVALID;

                HoistResult right = HoistExpression(std::move(node->Right), false, true, method, node, model, diagnostics);
                if (!right.Supported)
                    return HR_INVALID;

                node->Condition = std::move(cond.Expression);
                node->Left = std::move(left.Expression);
                node->Right = std::move(right.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, cond.Prefix);
                MovePrefix(result.Prefix, left.Prefix);
                MovePrefix(result.Prefix, right.Prefix);

                return result;
            }

            case SyntaxKind::CastExpression:
            {
                CastExpressionSyntax* node = static_cast<CastExpressionSyntax*>(expr.get());

                HoistResult inner = HoistExpression(std::move(node->Expression), false, conditional, method, node, model, diagnostics);
                if (!inner.Supported)
                    return HR_INVALID;

                node->Expression = std::move(inner.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, inner.Prefix);

                return result;
            }

            case SyntaxKind::IsExpression:
            {
                IsExpressionSyntax* node = static_cast<IsExpressionSyntax*>(expr.get());

                HoistResult inner = HoistExpression(std::move(node->Expression), false, conditional, method, node, model, diagnostics);
                if (!inner.Supported)
                    return HR_INVALID;

                node->Expression = std::move(inner.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, inner.Prefix);

                return result;
            }

            case SyntaxKind::RangeExpression:
            {
                RangeExpressionSyntax* node = static_cast<RangeExpressionSyntax*>(expr.get());

                HoistResult left = HoistExpression(std::move(node->Left), false, conditional, method, node, model, diagnostics);
                if (!left.Supported)
                    return HR_INVALID;

                HoistResult right = HoistExpression(std::move(node->Right), false, conditional, method, node, model, diagnostics);
                if (!right.Supported)
                    return HR_INVALID;

                node->Left = std::move(left.Expression);
                node->Right = std::move(right.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, left.Prefix);
                MovePrefix(result.Prefix, right.Prefix);

                return result;
            }

            case SyntaxKind::CollectionExpression:
            {
                CollectionExpressionSyntax* node = static_cast<CollectionExpressionSyntax*>(expr.get());

                HoistResult result;
                result.Expression = std::move(expr);

                for (auto& value : node->ValuesExpressions)
                {
                    HoistResult item = HoistExpression(std::move(value), false, conditional, method, node, model, diagnostics);
                    if (!item.Supported)
                        return HR_INVALID;

                    value = std::move(item.Expression);
                    MovePrefix(result.Prefix, item.Prefix);
                }

                return result;
            }

            case SyntaxKind::ObjectExpression:
            {
                ObjectExpressionSyntax* node = static_cast<ObjectExpressionSyntax*>(expr.get());

                HoistResult result;
                result.Expression = std::move(expr);

                if (node->ArgumentsList != nullptr)
                {
                    for (auto& arg : node->ArgumentsList->Arguments)
                    {
                        HoistResult item = HoistExpression(std::move(arg->Expression), false, conditional, method, arg.get(), model, diagnostics);
                        if (!item.Supported)
                            return HR_INVALID;

                        arg->Expression = std::move(item.Expression);
                        MovePrefix(result.Prefix, item.Prefix);
                    }
                }

                return result;
            }

            case SyntaxKind::IfExpression:
            {
                IfExpressionSyntax* node = static_cast<IfExpressionSyntax*>(expr.get());

                HoistResult cond = HoistExpression(std::move(node->Condition), false, conditional, method, node, model, diagnostics);
                if (!cond.Supported)
                    return HR_INVALID;

                HoistResult thenExpr = HoistExpression(std::move(node->ThenExpression), false, true, method, node, model, diagnostics);
                if (!thenExpr.Supported)
                    return HR_INVALID;

                HoistResult elseExpr = HoistExpression(std::move(node->ElseExpression), false, true, method, node, model, diagnostics);
                if (!elseExpr.Supported)
                    return HR_INVALID;

                node->Condition = std::move(cond.Expression);
                node->ThenExpression = std::move(thenExpr.Expression);
                node->ElseExpression = std::move(elseExpr.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, cond.Prefix);
                MovePrefix(result.Prefix, thenExpr.Prefix);
                MovePrefix(result.Prefix, elseExpr.Prefix);

                return result;
            }

            case SyntaxKind::SwitchExpression:
            {
                SwitchExpressionSyntax* node = static_cast<SwitchExpressionSyntax*>(expr.get());

                HoistResult switchValue = HoistExpression(std::move(node->Expression), false, conditional, method, node, model, diagnostics);
                if (!switchValue.Supported)
                    return HR_INVALID;

                node->Expression = std::move(switchValue.Expression);

                HoistResult result;
                result.Expression = std::move(expr);
                MovePrefix(result.Prefix, switchValue.Prefix);

                for (auto& arm : node->Arms)
                {
                    HoistResult pattern = HoistExpression(std::move(arm->Pattern), false, true, method, arm.get(), model, diagnostics);
                    if (!pattern.Supported)
                        return HR_INVALID;

                    HoistResult value = HoistExpression(std::move(arm->Expression), false, true, method, arm.get(), model, diagnostics);
                    if (!value.Supported)
                        return HR_INVALID;

                    arm->Pattern = std::move(pattern.Expression);
                    arm->Expression = std::move(value.Expression);

                    MovePrefix(result.Prefix, pattern.Prefix);
                    MovePrefix(result.Prefix, value.Prefix);
                }

                return result;
            }

            case SyntaxKind::MemberAccessExpression:
            case SyntaxKind::LinkedExpression:
            {
                LinkedExpressionNode* node = static_cast<LinkedExpressionNode*>(expr.get());

                HoistResult result;
                result.Expression = std::move(expr);

                if (node->PreviousExpression != nullptr)
                {
                    HoistResult prev = HoistExpression(std::move(node->PreviousExpression), false, conditional, method, node, model, diagnostics);
                    if (!prev.Supported)
                        return HR_INVALID;

                    node->PreviousExpression = std::move(prev.Expression);
                    MovePrefix(result.Prefix, prev.Prefix);
                }

                return result;
            }

            case SyntaxKind::InvokationExpression:
            {
                InvokationExpressionSyntax* node = static_cast<InvokationExpressionSyntax*>(expr.get());

                HoistResult result;
                result.Expression = std::move(expr);

                if (node->PreviousExpression != nullptr)
                {
                    HoistResult prev = HoistExpression(std::move(node->PreviousExpression), false, conditional, method, node, model, diagnostics);
                    if (!prev.Supported)
                        return HR_INVALID;

                    node->PreviousExpression = std::move(prev.Expression);
                    MovePrefix(result.Prefix, prev.Prefix);
                }

                if (node->ArgumentsList != nullptr)
                {
                    for (auto& arg : node->ArgumentsList->Arguments)
                    {
                        HoistResult item = HoistExpression(std::move(arg->Expression), false, conditional, method, arg.get(), model, diagnostics);
                        if (!item.Supported)
                            return HR_INVALID;

                        arg->Expression = std::move(item.Expression);
                        MovePrefix(result.Prefix, item.Prefix);
                    }
                }

                return result;
            }

            case SyntaxKind::IndexatorExpression:
            {
                IndexatorExpressionSyntax* node = static_cast<IndexatorExpressionSyntax*>(expr.get());

                HoistResult result;
                result.Expression = std::move(expr);

                if (node->PreviousExpression != nullptr)
                {
                    HoistResult prev = HoistExpression(std::move(node->PreviousExpression), false, conditional, method, node, model, diagnostics);
                    if (!prev.Supported)
                        return HR_INVALID;

                    node->PreviousExpression = std::move(prev.Expression);
                    MovePrefix(result.Prefix, prev.Prefix);
                }

                if (node->IndexatorList != nullptr)
                {
                    for (auto& arg : node->IndexatorList->Arguments)
                    {
                        HoistResult item = HoistExpression(std::move(arg->Expression), false, conditional, method, arg.get(), model, diagnostics);
                        if (!item.Supported)
                            return HR_INVALID;

                        arg->Expression = std::move(item.Expression);
                        MovePrefix(result.Prefix, item.Prefix);
                    }
                }

                return result;
            }

            case SyntaxKind::LiteralExpression:
            case SyntaxKind::TypeExpression:
            case SyntaxKind::LambdaExpression:
            {
                return HoistResult
                {
                    .Expression = std::move(expr),
                    .Prefix = {},
                    .Supported = true
                };
            }

            default:
            {
                // Unknown or unsupported expression kind; do not claim to support it.
                diagnostics.ReportError(SyntaxToken(), L"Await expression inside this expression form is not yet supported in async methods");
                return HR_INVALID;
            }
        }
    }

    static void HoistStatements(std::vector<std::unique_ptr<StatementSyntax>>& statements, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, bool& supported);
    static HoistResult HoistSubExpression(std::unique_ptr<ExpressionSyntax> expr, bool conditional, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics);
    static std::unique_ptr<StatementSyntax> HoistStatement(std::unique_ptr<StatementSyntax> statement, std::vector<std::unique_ptr<StatementSyntax>>& prefix, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, bool& supported);

    static void HoistStatements(std::vector<std::unique_ptr<StatementSyntax>>& statements, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, bool& supported)
    {
        for (std::size_t i = 0; i < statements.size(); ++i)
        {
            std::vector<std::unique_ptr<StatementSyntax>> localPrefix;
            auto rewritten = HoistStatement(std::move(statements[i]), localPrefix, method, parent, model, diagnostics, supported);
            if (!supported)
                return;

            statements[i] = std::move(rewritten);
            if (!localPrefix.empty())
            {
                for (auto& localStatement : localPrefix)
                    localStatement->Parent = parent;

                statements.insert(statements.begin() + i, std::make_move_iterator(localPrefix.begin()), std::make_move_iterator(localPrefix.end()));
                i += localPrefix.size();
            }
        }
    }

    static HoistResult HoistSubExpression(std::unique_ptr<ExpressionSyntax> expr, bool conditional, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics)
    {
        return HoistExpression(std::move(expr), true, conditional, method, parent, model, diagnostics);
    }

    static std::unique_ptr<StatementSyntax> HoistStatement(std::unique_ptr<StatementSyntax> statement, std::vector<std::unique_ptr<StatementSyntax>>& prefix, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, bool& supported)
    {
        if (statement == nullptr)
            return statement;

        supported = false;
        statement->Parent = parent;

        switch (statement->Kind)
        {
            case SyntaxKind::ExpressionStatement:
            {
                ExpressionStatementSyntax* node = static_cast<ExpressionStatementSyntax*>(statement.get());
                if (node->Expression != nullptr)
                {
                    if (node->Expression->Kind == SyntaxKind::AwaitExpression)
                    {
                        // Top-level await: hoist only inside the awaitable expression.
                        AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
                        HoistResult inner = HoistSubExpression(std::move(awaitExpr->Expression), false, method, awaitExpr, model, diagnostics);

                        supported = inner.Supported;
                        if (!supported)
                            return statement;

                        awaitExpr->Expression = std::move(inner.Expression);
                        MovePrefix(prefix, inner.Prefix);
                    }
                    else
                    {
                        HoistResult result = HoistExpression(std::move(node->Expression), false, false, method, node, model, diagnostics);
                        if (!result.Supported) { supported = false; return statement; }
                        node->Expression = std::move(result.Expression);
                        MovePrefix(prefix, result.Prefix);
                    }
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::VariableStatement:
            {
                VariableStatementSyntax* node = static_cast<VariableStatementSyntax*>(statement.get());
                if (node->Expression != nullptr)
                {
                    if (node->Expression->Kind == SyntaxKind::AwaitExpression)
                    {
                        AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
                        HoistResult inner = HoistSubExpression(std::move(awaitExpr->Expression), false, method, awaitExpr, model, diagnostics);
                        supported = inner.Supported;

                        if (!supported)
                            return statement;

                        awaitExpr->Expression = std::move(inner.Expression);
                        MovePrefix(prefix, inner.Prefix);
                    }
                    else
                    {
                        HoistResult result = HoistExpression(std::move(node->Expression), false, false, method, node, model, diagnostics);
                        supported = result.Supported;

                        if (!supported)
                            return statement;

                        node->Expression = std::move(result.Expression);
                        MovePrefix(prefix, result.Prefix);
                    }
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::ReturnStatement:
            {
                ReturnStatementSyntax* node = static_cast<ReturnStatementSyntax*>(statement.get());
                if (node->Expression != nullptr)
                {
                    if (node->Expression->Kind == SyntaxKind::AwaitExpression)
                    {
                        AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
                        HoistResult inner = HoistSubExpression(std::move(awaitExpr->Expression), false, method, awaitExpr, model, diagnostics);
                        supported = inner.Supported;

                        if (!supported)
                            return statement;

                        awaitExpr->Expression = std::move(inner.Expression);
                        MovePrefix(prefix, inner.Prefix);
                    }
                    else
                    {
                        HoistResult result = HoistExpression(std::move(node->Expression), false, false, method, node, model, diagnostics);
                        supported = result.Supported;

                        if (!supported)
                            return statement;

                        node->Expression = std::move(result.Expression);
                        MovePrefix(prefix, result.Prefix);
                    }
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::ThrowStatement:
            {
                ThrowStatementSyntax* node = static_cast<ThrowStatementSyntax*>(statement.get());
                if (node->Expression != nullptr)
                {
                    HoistResult result = HoistExpression(std::move(node->Expression), false, false, method, node, model, diagnostics);
                    supported = result.Supported;

                    if (!supported)
                        return statement;

                    node->Expression = std::move(result.Expression);
                    MovePrefix(prefix, result.Prefix);
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::IfStatement:
            case SyntaxKind::UnlessStatement:
            {
                ConditionalClauseSyntax* node = static_cast<ConditionalClauseSyntax*>(statement.get());
                if (node->ConditionExpression != nullptr)
                {
                    // If the condition is a top-level await expression statement, hoist it
                    // out into a compiler-generated variable so the await is at block level.
                    if (node->ConditionExpression->Kind == SyntaxKind::ExpressionStatement)
                    {
                        ExpressionStatementSyntax* condExprStmt = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
                        if (condExprStmt->Expression != nullptr && condExprStmt->Expression->Kind == SyntaxKind::AwaitExpression)
                        {
                            AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(condExprStmt->Expression.get());

                            HoistResult inner = HoistSubExpression(std::move(awaitExpr->Expression), false, method, awaitExpr, model, diagnostics);
                            supported = inner.Supported;

                            if (!supported)
                                return statement;

                            awaitExpr->Expression = std::move(inner.Expression);
                            MovePrefix(prefix, inner.Prefix);

                            static int condCounter = 0;
                            condCounter = 0;

                            TypeSymbol* resultType = (awaitExpr->GetResultMethod != nullptr && awaitExpr->GetResultMethod->ReturnType != nullptr)
                                ? const_cast<TypeSymbol*>(awaitExpr->GetResultMethod->ReturnType) : SymbolTable::Primitives::Any;

                            std::wstring varName = std::format(L"<a>__condTmp{}", condCounter++);
                            SyntaxToken nameToken(TokenType::Identifier, varName, awaitExpr->AwaitKeywordToken.Location);
                            SyntaxToken assignToken(TokenType::DeclareAssignOperator, L":=", awaitExpr->AwaitKeywordToken.Location);

                            std::unique_ptr<VariableStatementSyntax> varStmt = std::make_unique<VariableStatementSyntax>(nullptr, nameToken, assignToken, std::move(condExprStmt->Expression), parent);
                            VariableSymbol* varSymbol = model.Table->BindSymbolEx<VariableSymbol>(varStmt.get(), std::make_unique<VariableSymbol>(varName, resultType));

                            varSymbol->SlotIndex = static_cast<std::uint16_t>(method->GetEvalStackArgumentsCount() + method->AddVariableCount());
                            varSymbol->Parent = method;
                            varSymbol->FullName = method->FullName + L"." + varName;

                            prefix.push_back(std::move(varStmt));
                            condExprStmt->Expression = CreateVariableReference(varSymbol, condExprStmt);
                        }
                        else
                        {
                            std::vector<std::unique_ptr<StatementSyntax>> condPrefix;
                            std::unique_ptr<StatementSyntax> rewrittenCond = HoistStatement(std::move(node->ConditionExpression), condPrefix, method, node, model, diagnostics, supported);

                            if (!supported)
                                return statement;

                            MovePrefix(prefix, condPrefix);
                            node->ConditionExpression = std::move(rewrittenCond);
                        }
                    }
                    else
                    {
                        std::vector<std::unique_ptr<StatementSyntax>> condPrefix;
                        std::unique_ptr<StatementSyntax> rewrittenCond = HoistStatement(std::move(node->ConditionExpression), condPrefix, method, node, model, diagnostics, supported);

                        if (!supported)
                            return statement;

                        MovePrefix(prefix, condPrefix);
                        node->ConditionExpression = std::move(rewrittenCond);
                    }
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                if (!supported)
                    return statement;

                if (node->NextStatement != nullptr)
                {
                    std::unique_ptr<StatementSyntax> nextStmt(node->NextStatement.release());
                    std::vector<std::unique_ptr<StatementSyntax>> nextPrefix;
                    std::unique_ptr<StatementSyntax> rewrittenNext = HoistStatement(std::move(nextStmt), nextPrefix, method, parent, model, diagnostics, supported);

                    if (!supported)
                        return statement;

                    MovePrefix(prefix, nextPrefix);
                    rewrittenNext->Parent = node;
                    node->NextStatement.reset(static_cast<ConditionalClauseBaseSyntax*>(rewrittenNext.release()));
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::ElseStatement:
            {
                ElseStatementSyntax* node = static_cast<ElseStatementSyntax*>(statement.get());
                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                if (!supported)
                    return statement;

                if (node->NextStatement != nullptr)
                {
                    std::unique_ptr<StatementSyntax> nextStmt(node->NextStatement.release());
                    std::vector<std::unique_ptr<StatementSyntax>> nextPrefix;
                    std::unique_ptr<StatementSyntax> rewrittenNext = HoistStatement(std::move(nextStmt), nextPrefix, method, parent, model, diagnostics, supported);

                    if (!supported)
                        return statement;

                    MovePrefix(prefix, nextPrefix);
                    rewrittenNext->Parent = node;
                    node->NextStatement.reset(static_cast<ConditionalClauseBaseSyntax*>(rewrittenNext.release()));
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::WhileStatement:
            case SyntaxKind::UntilStatement:
            {
                auto* node = static_cast<WhileStatementSyntax*>(statement.get());
                if (node->ConditionExpression != nullptr)
                {
                    // Loop conditions are repeatedly evaluated; do not hoist awaits out of them.
                    HoistResult cond = HoistSubExpression(std::move(node->ConditionExpression), true, method, node, model, diagnostics);
                    supported = cond.Supported;

                    if (!supported)
                        return statement;

                    node->ConditionExpression = std::move(cond.Expression);
                    MovePrefix(prefix, cond.Prefix);
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                supported = true;
                return statement;
            }

            case SyntaxKind::ForStatement:
            {
                ForStatementSyntax* node = static_cast<ForStatementSyntax*>(statement.get());
                if (node->InitializerStatement != nullptr)
                {
                    std::vector<std::unique_ptr<StatementSyntax>> initPrefix;
                    auto rewrittenInit = HoistStatement(std::move(node->InitializerStatement), initPrefix, method, node, model, diagnostics, supported);

                    if (!supported)
                        return statement;

                    MovePrefix(prefix, initPrefix);
                    node->InitializerStatement = std::move(rewrittenInit);
                }

                if (node->ConditionExpression != nullptr)
                {
                    HoistResult cond = HoistSubExpression(std::move(node->ConditionExpression), true, method, node, model, diagnostics);
                    supported = cond.Supported;

                    if (!supported)
                        return statement;

                    node->ConditionExpression = std::move(cond.Expression);
                    MovePrefix(prefix, cond.Prefix);
                }

                if (node->AfterRepeatStatement != nullptr)
                {
                    std::vector<std::unique_ptr<StatementSyntax>> afterPrefix;
                    auto rewrittenAfter = HoistStatement(std::move(node->AfterRepeatStatement), afterPrefix, method, node, model, diagnostics, supported);

                    if (!supported)
                        return statement;

                    MovePrefix(prefix, afterPrefix);
                    node->AfterRepeatStatement = std::move(rewrittenAfter);
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                supported = true;
                return statement;
            }

            case SyntaxKind::TryStatement:
            {
                TryStatementSyntax* node = static_cast<TryStatementSyntax*>(statement.get());
                if (node->TryBlock != nullptr)
                    HoistStatements(node->TryBlock->Statements, method, node->TryBlock.get(), model, diagnostics, supported);

                if (!supported)
                    return statement;

                for (auto& clause : node->CatchClauses)
                {
                    if (clause->Body != nullptr)
                        HoistStatements(clause->Body->Statements, method, clause->Body.get(), model, diagnostics, supported);

                    if (!supported)
                        return statement;
                }

                supported = true;
                return statement;
            }

            case SyntaxKind::BreakStatement:
            case SyntaxKind::ContinueStatement:
            {
                // break/continue contain no await expressions and can be left
                // untouched; the regular emitter handles them inside loops.
                supported = true;
                return statement;
            }

            case SyntaxKind::ForEachStatement:
            {
                auto* node = static_cast<ForEachStatementSyntax*>(statement.get());

                if (node->RangeExpression != nullptr)
                {
                    HoistResult range = HoistSubExpression(std::move(node->RangeExpression), true, method, node, model, diagnostics);
                    supported = range.Supported;

                    if (!supported)
                        return statement;

                    node->RangeExpression = std::move(range.Expression);
                    MovePrefix(prefix, range.Prefix);
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                supported = true;
                return statement;
            }

            case SyntaxKind::ForInStatement:
            {
                auto* node = static_cast<ForInStatementSyntax*>(statement.get());

                if (node->RangeExpression != nullptr)
                {
                    HoistResult range = HoistSubExpression(std::move(node->RangeExpression), true, method, node, model, diagnostics);
                    supported = range.Supported;

                    if (!supported)
                        return statement;

                    node->RangeExpression = std::move(range.Expression);
                    MovePrefix(prefix, range.Prefix);
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                supported = true;
                return statement;
            }

            case SyntaxKind::DeferStatement:
            {
                // Defer needs proper finally lowering. Leave it untouched for now;
                // AsyncAnalysisPass will reject async methods that contain defer.
                supported = true;
                return statement;
            }

            default:
                return statement;
        }
    }
}

bool AsyncHoistingPass::Rewrite(StatementsBlockSyntax* body, MethodSymbol* method)
{
    if (body == nullptr)
        return true;

    bool supported = true;
    HoistStatements(body->Statements, method, body, Model, Diagnostics, supported);

    return supported;
}
