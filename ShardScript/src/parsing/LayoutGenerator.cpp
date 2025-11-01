#include <shard/parsing/LayoutGenerator.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

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
				Diagnostics.ReportError(token, "Recursive struct inlining");
				continue;
			}

			if (returnType->State == TypeLayoutingState::Unvisited)
				FixObjectLayout(semanticModel, returnType);

			objectInfo->MemoryBytesSize += returnType->MemoryBytesSize;
		}
	}

	objectInfo->State = TypeLayoutingState::Visited;
	return;
}