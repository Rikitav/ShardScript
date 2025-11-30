#include <shard/parsing/LayoutGenerator.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

void LayoutGenerator::Generate(SemanticModel& semanticModel)
{
	for (TypeSymbol* objectInfo : semanticModel.Table->GetTypeSymbols())
	{
		if (objectInfo->State == TypeLayoutingState::Visited)
			return;

		FixObjectLayout(semanticModel, objectInfo);
	}
}

void LayoutGenerator::FixObjectLayout(SemanticModel& semanticModel, TypeSymbol* objectInfo)
{
	objectInfo->State = TypeLayoutingState::Visiting;

	for (FieldSymbol* field : objectInfo->Fields)
	{
		TypeSymbol* returnType = field->ReturnType;
		if (returnType == nullptr)
			continue;

		if (returnType->IsReferenceType)
		{
			field->MemoryBytesOffset = objectInfo->MemoryBytesSize;
			objectInfo->MemoryBytesSize += sizeof(void*);
		}
		else
		{
			if (returnType->State == TypeLayoutingState::Visiting)
			{
				SyntaxToken token = static_cast<FieldDeclarationSyntax*>(semanticModel.Table->GetSyntaxNode(field))->IdentifierToken;
				Diagnostics.ReportError(token, L"Recursive struct inlining");
				continue;
			}

			if (returnType->State == TypeLayoutingState::Unvisited)
				FixObjectLayout(semanticModel, returnType);

			field->MemoryBytesOffset = objectInfo->MemoryBytesSize;
			objectInfo->MemoryBytesSize += returnType->MemoryBytesSize;
		}
	}

	if (objectInfo->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayInfo = static_cast<ArrayTypeSymbol*>(objectInfo);
		if (arrayInfo->UnderlayingType->State == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, arrayInfo->UnderlayingType);

		objectInfo->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->MemoryBytesSize * arrayInfo->Size;
	}

	objectInfo->State = TypeLayoutingState::Visited;
	return;
}