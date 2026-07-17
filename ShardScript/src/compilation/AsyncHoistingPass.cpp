#include <shard/compilation/AsyncHoistingPass.hpp>

#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
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
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>

#include <format>
#include <memory>
#include <vector>

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

    // ------------------------------------------------------------------------
    // Helpers for lowering expressions that contain await into statement form.
    // ------------------------------------------------------------------------

    class AwaitContainsVisitor final : public SyntaxVisitor
    {
    public:
        bool Found = false;

        inline AwaitContainsVisitor(SemanticModel& model, DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics) { }

    protected:
        void VisitAwaitExpression(AwaitExpressionSyntax*) override
        {
            Found = true;
        }

        void VisitLambdaExpression(LambdaExpressionSyntax*) override
        {
            // Await inside a lambda body belongs to a separate async method,
            // not to the expression being evaluated here.
        }
    };

    static bool ContainsAwait(ExpressionSyntax* expr, SemanticModel& model, DiagnosticsContext& diagnostics)
    {
        if (expr == nullptr)
            return false;

        AwaitContainsVisitor visitor(model, diagnostics);
        visitor.VisitExpression(expr);
        return visitor.Found;
    }

    static bool ContainsAwait(StatementSyntax* stmt, SemanticModel& model, DiagnosticsContext& diagnostics)
    {
        if (stmt == nullptr)
            return false;

        AwaitContainsVisitor visitor(model, diagnostics);
        visitor.VisitStatement(stmt);
        return visitor.Found;
    }

    static SyntaxToken MakeAnonymousToken(const std::wstring& word, TokenType type = TokenType::Identifier)
    {
        return SyntaxToken(type, word, shard::TextLocation());
    }

    static std::unique_ptr<LiteralExpressionSyntax> MakeBoolLiteral(bool value, SyntaxNode* parent, SemanticModel& model)
    {
        SyntaxToken token = MakeAnonymousToken(value ? L"true" : L"false", TokenType::BooleanLiteral);
        auto node = std::make_unique<LiteralExpressionSyntax>(token, parent);

        auto symbol = std::make_unique<LiteralSymbol>(token.Type);
        symbol->AsBooleanValue = value;
        model.Table->BindSymbol(node.get(), std::move(symbol));

        return node;
    }

    static VariableSymbol* DeclareTemporaryVariable(VariableStatementSyntax* syntax, MethodSymbol* method, const std::wstring& baseName, TypeSymbol* type, SemanticModel& model)
    {
        static int counter = 0;
        std::wstring name = baseName + std::to_wstring(counter++);

        auto symbol = std::make_unique<VariableSymbol>(name, type);
        VariableSymbol* result = model.Table->BindSymbolEx<VariableSymbol>(syntax, std::move(symbol));
        result->SlotIndex = static_cast<std::uint16_t>(method->GetEvalStackArgumentsCount() + method->AddVariableCount());
        result->Parent = method;
        result->FullName = method->FullName + L"." + name;
        return result;
    }

    static std::unique_ptr<ExpressionStatementSyntax> CreateAssignmentStatement(VariableSymbol* variable, std::unique_ptr<ExpressionSyntax> value, SyntaxNode* parent)
    {
        auto binary = std::make_unique<BinaryExpressionSyntax>(MakeAnonymousToken(L"=", TokenType::AssignOperator), parent);
        binary->Left = CreateVariableReference(variable, binary.get());
        binary->Right = std::move(value);

        auto statement = std::make_unique<ExpressionStatementSyntax>(parent);
        statement->Expression = std::move(binary);
        return statement;
    }

    static std::unique_ptr<VariableStatementSyntax> CreateVariableStatement(VariableSymbol* variable, std::unique_ptr<ExpressionSyntax> initializer, SyntaxNode* parent)
    {
        SyntaxToken nameToken = MakeAnonymousToken(variable->Name);
        SyntaxToken assignToken = MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator);
        return std::make_unique<VariableStatementSyntax>(nullptr, nameToken, assignToken, std::move(initializer), parent);
    }

    // Forward declarations.
    static HoistResult HoistExpression(std::unique_ptr<ExpressionSyntax> expr, bool allowTopLevelAwait, bool conditional, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics);
    static bool SpillExpressionInto(std::unique_ptr<ExpressionSyntax> expr, VariableSymbol* destination, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, std::vector<std::unique_ptr<StatementSyntax>>& statements);
    static bool SpillTernaryLike(std::unique_ptr<ExpressionSyntax> condition, std::unique_ptr<ExpressionSyntax> whenTrue, std::unique_ptr<ExpressionSyntax> whenFalse, VariableSymbol* destination, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, std::vector<std::unique_ptr<StatementSyntax>>& statements);

    static std::unique_ptr<StatementsBlockSyntax> WrapStatements(std::vector<std::unique_ptr<StatementSyntax>>& statements, SyntaxNode* parent)
    {
        auto block = std::make_unique<StatementsBlockSyntax>(parent);
        for (auto& stmt : statements)
        {
            stmt->Parent = block.get();
            block->Statements.push_back(std::move(stmt));
        }

        statements.clear();
        return block;
    }

    static bool SpillCondition(std::unique_ptr<ExpressionSyntax> expr, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, std::vector<std::unique_ptr<StatementSyntax>>& statements, VariableSymbol*& outVariable, std::unique_ptr<ExpressionSyntax>& outExpression)
    {
        if (!ContainsAwait(expr.get(), model, diagnostics))
        {
            outVariable = nullptr;
            outExpression = std::move(expr);
            return true;
        }

        auto declaration = std::make_unique<VariableStatementSyntax>(nullptr,
            MakeAnonymousToken(L""),
            MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator),
            MakeBoolLiteral(false, parent, model),
            parent);

        VariableSymbol* destination = DeclareTemporaryVariable(declaration.get(), method, L"<a>__cond", SymbolTable::Primitives::Boolean, model);
        declaration->IdentifierToken = MakeAnonymousToken(destination->Name);

        if (!SpillExpressionInto(std::move(expr), destination, method, parent, model, diagnostics, statements))
            return false;

        statements.insert(statements.begin(), std::move(declaration));
        outVariable = destination;
        outExpression = CreateVariableReference(destination, parent);
        return true;
    }

    static bool SpillExpressionInto(std::unique_ptr<ExpressionSyntax> expr, VariableSymbol* destination, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, std::vector<std::unique_ptr<StatementSyntax>>& statements)
    {
        if (expr == nullptr)
            return true;

        // Re-root the expression in the new generated context so that any
        // parent pointers left pointing at an intermediate node that is about
        // to be deleted (e.g. a short-circuit binary expression) do not dangle.
        expr->Parent = parent;

        switch (expr->Kind)
        {
            case SyntaxKind::AwaitExpression:
            {
                auto* node = static_cast<AwaitExpressionSyntax*>(expr.release());

                HoistResult awaitable = HoistExpression(std::move(node->Expression), false, false, method, node, model, diagnostics);
                if (!awaitable.Supported)
                {
                    delete node;
                    return false;
                }

                node->Expression = std::move(awaitable.Expression);

                TypeSymbol* resultType = (node->GetResultMethod != nullptr && node->GetResultMethod->ReturnType != nullptr)
                    ? const_cast<TypeSymbol*>(node->GetResultMethod->ReturnType)
                    : SymbolTable::Primitives::Any;

                auto tempDeclaration = std::make_unique<VariableStatementSyntax>(nullptr,
                    MakeAnonymousToken(L""),
                    MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator),
                    std::unique_ptr<ExpressionSyntax>(node),
                    parent);

                VariableSymbol* tempVariable = DeclareTemporaryVariable(tempDeclaration.get(), method, L"<a>__awaitTmp", resultType, model);
                tempDeclaration->IdentifierToken = MakeAnonymousToken(tempVariable->Name);

                MovePrefix(statements, awaitable.Prefix);
                statements.push_back(std::move(tempDeclaration));
                statements.push_back(CreateAssignmentStatement(destination, CreateVariableReference(tempVariable, parent), parent));
                return true;
            }

            case SyntaxKind::BinaryExpression:
            {
                auto* node = static_cast<BinaryExpressionSyntax*>(expr.release());

                bool isAnd = node->OperatorToken.Type == TokenType::AndOperator;
                bool isOr = node->OperatorToken.Type == TokenType::OrOperator;

                if (isAnd || isOr)
                {
                    std::unique_ptr<ExpressionSyntax> left = std::move(node->Left);
                    std::unique_ptr<ExpressionSyntax> right = std::move(node->Right);
                    delete node;

                    if (!SpillExpressionInto(std::move(left), destination, method, parent, model, diagnostics, statements))
                        return false;

                    std::vector<std::unique_ptr<StatementSyntax>> branchStatements;
                    if (!SpillExpressionInto(std::move(right), destination, method, parent, model, diagnostics, branchStatements))
                        return false;

                    auto conditionReference = CreateVariableReference(destination, nullptr);
                    auto conditionStatement = std::make_unique<ExpressionStatementSyntax>(nullptr);
                    conditionStatement->Expression = std::move(conditionReference);

                    if (isAnd)
                    {
                        auto ifStmt = std::make_unique<IfStatementSyntax>(parent);
                        auto branchBlock = std::make_unique<StatementsBlockSyntax>(ifStmt.get());
                        for (auto& stmt : branchStatements)
                        {
                            stmt->Parent = branchBlock.get();
                            branchBlock->Statements.push_back(std::move(stmt));
                        }

                        ifStmt->ConditionExpression = std::move(conditionStatement);
                        ifStmt->ConditionExpression->Parent = ifStmt.get();
                        ifStmt->StatementsBlock = std::move(branchBlock);
                        statements.push_back(std::move(ifStmt));
                    }
                    else
                    {
                        auto unlessStmt = std::make_unique<UnlessStatementSyntax>(parent);
                        auto branchBlock = std::make_unique<StatementsBlockSyntax>(unlessStmt.get());
                        for (auto& stmt : branchStatements)
                        {
                            stmt->Parent = branchBlock.get();
                            branchBlock->Statements.push_back(std::move(stmt));
                        }

                        unlessStmt->ConditionExpression = std::move(conditionStatement);
                        unlessStmt->ConditionExpression->Parent = unlessStmt.get();
                        unlessStmt->StatementsBlock = std::move(branchBlock);
                        statements.push_back(std::move(unlessStmt));
                    }

                    return true;
                }

                HoistResult left = HoistExpression(std::move(node->Left), false, false, method, node, model, diagnostics);
                if (!left.Supported)
                {
                    delete node;
                    return false;
                }

                HoistResult right = HoistExpression(std::move(node->Right), false, false, method, node, model, diagnostics);
                if (!right.Supported)
                {
                    delete node;
                    return false;
                }

                node->Left = std::move(left.Expression);
                node->Right = std::move(right.Expression);

                MovePrefix(statements, left.Prefix);
                MovePrefix(statements, right.Prefix);
                statements.push_back(CreateAssignmentStatement(destination, std::unique_ptr<ExpressionSyntax>(node), parent));
                return true;
            }

            case SyntaxKind::UnaryExpression:
            {
                auto* node = static_cast<UnaryExpressionSyntax*>(expr.release());
                SyntaxToken opToken = node->OperatorToken;
                bool isRightDetermined = node->IsRightDetermined;
                std::unique_ptr<ExpressionSyntax> inner = std::move(node->Expression);
                delete node;

                if (!SpillExpressionInto(std::move(inner), destination, method, parent, model, diagnostics, statements))
                    return false;

                auto unary = std::make_unique<UnaryExpressionSyntax>(opToken, isRightDetermined, parent);
                unary->Expression = CreateVariableReference(destination, unary.get());
                statements.push_back(CreateAssignmentStatement(destination, std::move(unary), parent));
                return true;
            }

            case SyntaxKind::TernaryExpression:
            {
                auto* node = static_cast<TernaryExpressionSyntax*>(expr.release());
                std::unique_ptr<ExpressionSyntax> condition = std::move(node->Condition);
                std::unique_ptr<ExpressionSyntax> left = std::move(node->Left);
                std::unique_ptr<ExpressionSyntax> right = std::move(node->Right);
                delete node;

                return SpillTernaryLike(std::move(condition), std::move(left), std::move(right), destination, method, parent, model, diagnostics, statements);
            }

            case SyntaxKind::IfExpression:
            {
                auto* node = static_cast<IfExpressionSyntax*>(expr.release());
                std::unique_ptr<ExpressionSyntax> condition = std::move(node->Condition);
                std::unique_ptr<ExpressionSyntax> thenExpr = std::move(node->ThenExpression);
                std::unique_ptr<ExpressionSyntax> elseExpr = std::move(node->ElseExpression);
                delete node;

                return SpillTernaryLike(std::move(condition), std::move(thenExpr), std::move(elseExpr), destination, method, parent, model, diagnostics, statements);
            }

            default:
            {
                HoistResult result = HoistExpression(std::move(expr), false, false, method, parent, model, diagnostics);
                if (!result.Supported)
                    return false;

                MovePrefix(statements, result.Prefix);
                statements.push_back(CreateAssignmentStatement(destination, std::move(result.Expression), parent));
                return true;
            }
        }
    }

    static bool SpillTernaryLike(std::unique_ptr<ExpressionSyntax> condition, std::unique_ptr<ExpressionSyntax> whenTrue, std::unique_ptr<ExpressionSyntax> whenFalse, VariableSymbol* destination, MethodSymbol* method, SyntaxNode* parent, SemanticModel& model, DiagnosticsContext& diagnostics, std::vector<std::unique_ptr<StatementSyntax>>& statements)
    {
        auto conditionDeclaration = std::make_unique<VariableStatementSyntax>(nullptr,
            MakeAnonymousToken(L""),
            MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator),
            MakeBoolLiteral(false, parent, model),
            parent);

        VariableSymbol* conditionVariable = DeclareTemporaryVariable(conditionDeclaration.get(), method, L"<a>__cond", SymbolTable::Primitives::Boolean, model);
        conditionDeclaration->IdentifierToken = MakeAnonymousToken(conditionVariable->Name);

        std::vector<std::unique_ptr<StatementSyntax>> conditionStatements;
        if (!SpillExpressionInto(std::move(condition), conditionVariable, method, parent, model, diagnostics, conditionStatements))
            return false;

        conditionStatements.insert(conditionStatements.begin(), std::move(conditionDeclaration));
        MovePrefix(statements, conditionStatements);

        std::vector<std::unique_ptr<StatementSyntax>> thenStatements;
        if (!SpillExpressionInto(std::move(whenTrue), destination, method, parent, model, diagnostics, thenStatements))
            return false;

        std::vector<std::unique_ptr<StatementSyntax>> elseStatements;
        if (!SpillExpressionInto(std::move(whenFalse), destination, method, parent, model, diagnostics, elseStatements))
            return false;

        auto ifStmt = std::make_unique<IfStatementSyntax>(parent);

        auto thenBlock = std::make_unique<StatementsBlockSyntax>(ifStmt.get());
        for (auto& stmt : thenStatements)
        {
            stmt->Parent = thenBlock.get();
            thenBlock->Statements.push_back(std::move(stmt));
        }

        auto elseClause = std::make_unique<ElseStatementSyntax>(ifStmt.get());
        auto elseBlock = std::make_unique<StatementsBlockSyntax>(elseClause.get());
        for (auto& stmt : elseStatements)
        {
            stmt->Parent = elseBlock.get();
            elseBlock->Statements.push_back(std::move(stmt));
        }
        elseClause->StatementsBlock = std::move(elseBlock);

        auto conditionReference = CreateVariableReference(conditionVariable, nullptr);
        auto conditionStatement = std::make_unique<ExpressionStatementSyntax>(nullptr);
        conditionStatement->Expression = std::move(conditionReference);

        ifStmt->ConditionExpression = std::move(conditionStatement);
        ifStmt->ConditionExpression->Parent = ifStmt.get();
        ifStmt->StatementsBlock = std::move(thenBlock);
        ifStmt->NextStatement = std::move(elseClause);

        statements.push_back(std::move(ifStmt));
        return true;
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
                if (node->ConditionExpression != nullptr && node->ConditionExpression->Kind == SyntaxKind::ExpressionStatement)
                {
                    ExpressionStatementSyntax* condExprStmt = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
                    if (condExprStmt->Expression != nullptr && ContainsAwait(condExprStmt->Expression.get(), model, diagnostics))
                    {
                        std::vector<std::unique_ptr<StatementSyntax>> condPrefix;
                        VariableSymbol* condVariable = nullptr;
                        std::unique_ptr<ExpressionSyntax> replacement;
                        if (!SpillCondition(std::move(condExprStmt->Expression), method, parent, model, diagnostics, condPrefix, condVariable, replacement))
                        {
                            supported = false;
                            return statement;
                        }

                        MovePrefix(prefix, condPrefix);
                        condExprStmt->Expression = std::move(replacement);
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
                bool isUntil = (statement->Kind == SyntaxKind::UntilStatement);

                if (node->ConditionExpression != nullptr && ContainsAwait(node->ConditionExpression.get(), model, diagnostics))
                {
                    auto newWhile = std::make_unique<WhileStatementSyntax>(parent);
                    auto newBody = std::make_unique<StatementsBlockSyntax>(newWhile.get());
                    newWhile->ConditionExpression = MakeBoolLiteral(true, newWhile.get(), model);

                    VariableSymbol* condVariable = nullptr;
                    std::unique_ptr<ExpressionSyntax> condRef;
                    std::vector<std::unique_ptr<StatementSyntax>> spillStatements;
                    if (!SpillCondition(std::move(node->ConditionExpression), method, newBody.get(), model, diagnostics, spillStatements, condVariable, condRef))
                    {
                        supported = false;
                        return statement;
                    }

                    for (auto& stmt : spillStatements)
                    {
                        stmt->Parent = newBody.get();
                        newBody->Statements.push_back(std::move(stmt));
                    }

                    std::unique_ptr<ExpressionSyntax> breakTest;
                    if (isUntil)
                    {
                        breakTest = CreateVariableReference(condVariable, nullptr);
                    }
                    else
                    {
                        auto notExpr = std::make_unique<UnaryExpressionSyntax>(MakeAnonymousToken(L"!", TokenType::NotOperator), false, nullptr);
                        notExpr->Expression = CreateVariableReference(condVariable, notExpr.get());
                        breakTest = std::move(notExpr);
                    }

                    auto breakCondStmt = std::make_unique<ExpressionStatementSyntax>(nullptr);
                    breakCondStmt->Expression = std::move(breakTest);

                    auto breakIf = std::make_unique<IfStatementSyntax>(newBody.get());
                    breakIf->ConditionExpression = std::move(breakCondStmt);
                    breakIf->ConditionExpression->Parent = breakIf.get();
                    auto breakBody = std::make_unique<StatementsBlockSyntax>(breakIf.get());
                    auto breakStmt = std::make_unique<BreakStatementSyntax>(breakBody.get());
                    breakBody->Statements.push_back(std::move(breakStmt));
                    breakIf->StatementsBlock = std::move(breakBody);
                    newBody->Statements.push_back(std::move(breakIf));

                    if (node->StatementsBlock != nullptr)
                    {
                        for (auto& stmt : node->StatementsBlock->Statements)
                        {
                            stmt->Parent = newBody.get();
                            newBody->Statements.push_back(std::move(stmt));
                        }
                    }

                    newWhile->StatementsBlock = std::move(newBody);
                    HoistStatements(newWhile->StatementsBlock->Statements, method, newWhile->StatementsBlock.get(), model, diagnostics, supported);
                    if (!supported)
                        return statement;

                    statement = std::move(newWhile);
                    supported = true;
                    return statement;
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

                bool conditionHasAwait = node->ConditionExpression != nullptr && ContainsAwait(node->ConditionExpression.get(), model, diagnostics);
                bool afterHasAwait = node->AfterRepeatStatement != nullptr && ContainsAwait(node->AfterRepeatStatement.get(), model, diagnostics);

                if (conditionHasAwait || afterHasAwait)
                {
                    if (node->StatementsBlock == nullptr)
                        node->StatementsBlock = std::make_unique<StatementsBlockSyntax>(node);

                    std::vector<std::unique_ptr<StatementSyntax>> newBodyStatements;

                    if (conditionHasAwait)
                    {
                        VariableSymbol* condVariable = nullptr;
                        std::unique_ptr<ExpressionSyntax> condRef;
                        std::vector<std::unique_ptr<StatementSyntax>> spillStatements;
                        if (!SpillCondition(std::move(node->ConditionExpression), method, node->StatementsBlock.get(), model, diagnostics, spillStatements, condVariable, condRef))
                        {
                            supported = false;
                            return statement;
                        }

                        for (auto& stmt : spillStatements)
                        {
                            stmt->Parent = node->StatementsBlock.get();
                            newBodyStatements.push_back(std::move(stmt));
                        }

                        auto notExpr = std::make_unique<UnaryExpressionSyntax>(MakeAnonymousToken(L"!", TokenType::NotOperator), false, nullptr);
                        notExpr->Expression = CreateVariableReference(condVariable, notExpr.get());

                        auto breakCondStmt = std::make_unique<ExpressionStatementSyntax>(nullptr);
                        breakCondStmt->Expression = std::move(notExpr);

                        auto breakIf = std::make_unique<IfStatementSyntax>(node->StatementsBlock.get());
                        breakIf->ConditionExpression = std::move(breakCondStmt);
                        breakIf->ConditionExpression->Parent = breakIf.get();
                        auto breakBody = std::make_unique<StatementsBlockSyntax>(breakIf.get());
                        auto breakStmt = std::make_unique<BreakStatementSyntax>(breakBody.get());
                        breakBody->Statements.push_back(std::move(breakStmt));
                        breakIf->StatementsBlock = std::move(breakBody);
                        newBodyStatements.push_back(std::move(breakIf));

                        node->ConditionExpression = MakeBoolLiteral(true, node, model);
                    }

                    for (auto& stmt : node->StatementsBlock->Statements)
                    {
                        stmt->Parent = node->StatementsBlock.get();
                        newBodyStatements.push_back(std::move(stmt));
                    }

                    if (afterHasAwait && node->AfterRepeatStatement != nullptr)
                    {
                        node->AfterRepeatStatement->Parent = node->StatementsBlock.get();
                        newBodyStatements.push_back(std::move(node->AfterRepeatStatement));
                        node->AfterRepeatStatement = nullptr;
                    }

                    node->StatementsBlock->Statements = std::move(newBodyStatements);
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);
                    if (!supported)
                        return statement;

                    supported = true;
                    return statement;
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

                if (node->RangeExpression != nullptr && ContainsAwait(node->RangeExpression.get(), model, diagnostics))
                {
                    if (node->RangeExpression->Kind == SyntaxKind::AwaitExpression)
                    {
                        auto* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->RangeExpression.get());
                        HoistResult awaitable = HoistExpression(std::move(awaitExpr->Expression), false, false, method, awaitExpr, model, diagnostics);
                        if (!awaitable.Supported)
                        {
                            supported = false;
                            return statement;
                        }

                        awaitExpr->Expression = std::move(awaitable.Expression);

                        TypeSymbol* resultType = (awaitExpr->GetResultMethod != nullptr && awaitExpr->GetResultMethod->ReturnType != nullptr)
                            ? const_cast<TypeSymbol*>(awaitExpr->GetResultMethod->ReturnType)
                            : SymbolTable::Primitives::Any;

                        auto rangeVarStmt = std::make_unique<VariableStatementSyntax>(nullptr,
                            MakeAnonymousToken(L""),
                            MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator),
                            std::move(node->RangeExpression),
                            parent);

                        VariableSymbol* rangeVariable = DeclareTemporaryVariable(rangeVarStmt.get(), method, L"<a>__range", resultType, model);
                        rangeVarStmt->IdentifierToken = MakeAnonymousToken(rangeVariable->Name);

                        MovePrefix(prefix, awaitable.Prefix);
                        prefix.push_back(std::move(rangeVarStmt));
                        node->RangeExpression = CreateVariableReference(rangeVariable, node);
                    }
                    else
                    {
                        HoistResult range = HoistExpression(std::move(node->RangeExpression), false, false, method, node, model, diagnostics);
                        supported = range.Supported;

                        if (!supported)
                            return statement;

                        node->RangeExpression = std::move(range.Expression);
                        MovePrefix(prefix, range.Prefix);
                    }
                }

                if (node->StatementsBlock != nullptr)
                    HoistStatements(node->StatementsBlock->Statements, method, node->StatementsBlock.get(), model, diagnostics, supported);

                supported = true;
                return statement;
            }

            case SyntaxKind::ForInStatement:
            {
                auto* node = static_cast<ForInStatementSyntax*>(statement.get());

                if (node->RangeExpression != nullptr && ContainsAwait(node->RangeExpression.get(), model, diagnostics))
                {
                    if (node->RangeExpression->Kind == SyntaxKind::AwaitExpression)
                    {
                        auto* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->RangeExpression.get());
                        HoistResult awaitable = HoistExpression(std::move(awaitExpr->Expression), false, false, method, awaitExpr, model, diagnostics);
                        if (!awaitable.Supported)
                        {
                            supported = false;
                            return statement;
                        }

                        awaitExpr->Expression = std::move(awaitable.Expression);

                        TypeSymbol* resultType = (awaitExpr->GetResultMethod != nullptr && awaitExpr->GetResultMethod->ReturnType != nullptr)
                            ? const_cast<TypeSymbol*>(awaitExpr->GetResultMethod->ReturnType)
                            : SymbolTable::Primitives::Any;

                        auto rangeVarStmt = std::make_unique<VariableStatementSyntax>(nullptr,
                            MakeAnonymousToken(L""),
                            MakeAnonymousToken(L":=", TokenType::DeclareAssignOperator),
                            std::move(node->RangeExpression),
                            parent);

                        VariableSymbol* rangeVariable = DeclareTemporaryVariable(rangeVarStmt.get(), method, L"<a>__range", resultType, model);
                        rangeVarStmt->IdentifierToken = MakeAnonymousToken(rangeVariable->Name);

                        MovePrefix(prefix, awaitable.Prefix);
                        prefix.push_back(std::move(rangeVarStmt));
                        node->RangeExpression = CreateVariableReference(rangeVariable, node);
                    }
                    else
                    {
                        HoistResult range = HoistExpression(std::move(node->RangeExpression), false, false, method, node, model, diagnostics);
                        supported = range.Supported;

                        if (!supported)
                            return statement;

                        node->RangeExpression = std::move(range.Expression);
                        MovePrefix(prefix, range.Prefix);
                    }
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
