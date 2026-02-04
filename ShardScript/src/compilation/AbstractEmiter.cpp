#include <shard/compilation/AbstractEmiter.h>
#include <shard/syntax/symbols/LiteralSymbol.h>

using namespace shard;

static void SetEntryPoint(DiagnosticsContext& diagnostics, std::vector<MethodSymbol*>& entryPointCandidates, SymbolTable* table, ProgramVirtualImage& program)
{
	if (entryPointCandidates.empty())
	{
		diagnostics.ReportError(SyntaxToken(), L"Entry point for script not found");
		return;
	}

	if (entryPointCandidates.size() > 1)
	{
		for (MethodSymbol* entry : entryPointCandidates)
		{
			MethodDeclarationSyntax* decl = static_cast<MethodDeclarationSyntax*>(table->GetSyntaxNode(entry));
			diagnostics.ReportError(decl->IdentifierToken, L"Script cannot have multiple entry points");
		}

		return;
	}

	program.EntryPoint = entryPointCandidates.front();
	return;
}

void AbstractEmiter::VisitSyntaxTree(SyntaxTree& tree)
{
	for (CompilationUnitSyntax* unit : tree.CompilationUnits)
		VisitCompilationUnit(unit);

	SetEntryPoint(Diagnostics, EntryPointCandidates, Table, Program);
}

void AbstractEmiter::VisitMethodDeclaration(MethodDeclarationSyntax *const node)
{
	GeneratingFor = LookupSymbol<MethodSymbol>(node);

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitConstructorDeclaration(ConstructorDeclarationSyntax *const node)
{
	GeneratingFor = LookupSymbol<ConstructorSymbol>(node);

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitAccessorDeclaration(AccessorDeclarationSyntax *const node)
{
	GeneratingFor = LookupSymbol<AccessorSymbol>(node);

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitExpressionStatement(ExpressionStatementSyntax *const node)
{
	VisitExpression(node->Expression);
}

void AbstractEmiter::VisitVariableStatement(VariableStatementSyntax *const node)
{
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->AssignToken, L"Emiting target not found");
		return;
	}

	VariableSymbol* var = LookupSymbol<VariableSymbol>(node);
	
	VisitExpression(node->Expression);
	Generator.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
}

void AbstractEmiter::VisitReturnStatement(ReturnStatementSyntax *const node)
{
}

void AbstractEmiter::VisitThrowStatement(ThrowStatementSyntax *const node)
{
}

void AbstractEmiter::VisitBreakStatement(BreakStatementSyntax *const node)
{
}

void AbstractEmiter::VisitContinueStatement(ContinueStatementSyntax *const node)
{
}

void AbstractEmiter::VisitWhileStatement(WhileStatementSyntax *const node)
{
}

void AbstractEmiter::VisitForStatement(ForStatementSyntax *const node)
{
}

void AbstractEmiter::VisitUntilStatement(UntilStatementSyntax *const node)
{
}

void AbstractEmiter::VisitConditionalClause(ConditionalClauseBaseSyntax *const node)
{
}

void AbstractEmiter::VisitIfStatement(IfStatementSyntax *const node)
{
}

void AbstractEmiter::VisitUnlessStatement(UnlessStatementSyntax *const node)
{
}

void AbstractEmiter::VisitElseStatement(ElseStatementSyntax *const node)
{
}

void AbstractEmiter::VisitLiteralExpression(LiteralExpressionSyntax *const node)
{
	LiteralSymbol* const symbol = static_cast<LiteralSymbol*>(Table->LookupSymbol(node));
	switch (symbol->LiteralType)
	{
		case TokenType::NullLiteral:
		{
			Generator.EmitLoadConstNull(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::CharLiteral:
		{
			Generator.EmitLoadConstChar16(GeneratingFor->ExecutableByteCode, node->LiteralToken.Word[0]);
			break;
		}

		case TokenType::StringLiteral:
		{
			Generator.EmitLoadConstString(GeneratingFor->ExecutableByteCode, Program.DataSection, node->LiteralToken.Word.data());
			break;
		}

		case TokenType::NumberLiteral:
		{
			Generator.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, symbol->AsIntegerValue);
			break;
		}

		case TokenType::DoubleLiteral:
		{
			Generator.EmitLoadConstDouble64(GeneratingFor->ExecutableByteCode, symbol->AsDoubleValue);
			break;
		}

		case TokenType::BooleanLiteral:
		{
			Generator.EmitLoadConstBool(GeneratingFor->ExecutableByteCode, symbol->AsBooleanValue);
			break;
		}
	}
}

void AbstractEmiter::VisitBinaryExpression(BinaryExpressionSyntax *const node)
{
	VisitExpression(node->Left);
	VisitExpression(node->Right);

	switch (node->OperatorToken.Type)
	{
		case TokenType::AddOperator:
		{
			Generator.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			break;
		}
	}
}

void AbstractEmiter::VisitUnaryExpression(UnaryExpressionSyntax *const node)
{
}

void AbstractEmiter::VisitObjectCreationExpression(ObjectExpressionSyntax *const node)
{
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax *const node)
{
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax *const node)
{
}

void AbstractEmiter::VisitTernaryExpression(TernaryExpressionSyntax *const node)
{
}

void AbstractEmiter::VisitInvocationExpression(InvokationExpressionSyntax *const node)
{
	VisitExpression(node->PreviousExpression);
	Generator.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitMemberAccessExpression(MemberAccessExpressionSyntax *const node)
{
	VisitExpression(node->PreviousExpression);

}

void AbstractEmiter::VisitIndexatorExpression(IndexatorExpressionSyntax *const node)
{
}
