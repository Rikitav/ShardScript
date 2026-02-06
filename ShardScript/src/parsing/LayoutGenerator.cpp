#include <shard/parsing/LayoutGenerator.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>

using namespace shard;

void LayoutGenerator::Generate(SemanticModel& semanticModel)
{
	for (TypeSymbol* objectInfo : semanticModel.Table->GetTypeSymbols())
	{
		if (objectInfo->State == TypeLayoutingState::Visited)
			continue;

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

		if (!returnType->IsReferenceType)
		{
			if (returnType->State == TypeLayoutingState::Visiting)
			{
				SyntaxToken token = static_cast<FieldDeclarationSyntax*>(semanticModel.Table->GetSyntaxNode(field))->IdentifierToken;
				Diagnostics.ReportError(token, L"Recursive struct inlining");
				continue;
			}
		}

		if (returnType->State == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, returnType);

		field->MemoryBytesOffset = objectInfo->MemoryBytesSize;
		objectInfo->MemoryBytesSize += returnType->GetInlineSize();
	}

	if (objectInfo->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayInfo = static_cast<ArrayTypeSymbol*>(objectInfo);
		if (arrayInfo->UnderlayingType->State == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, arrayInfo->UnderlayingType);

		objectInfo->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->MemoryBytesSize * arrayInfo->Size;
	}

	if (objectInfo->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericInfo = static_cast<GenericTypeSymbol*>(objectInfo);
		if (genericInfo->UnderlayingType->State == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, genericInfo->UnderlayingType);

		genericInfo->MemoryBytesSize = genericInfo->UnderlayingType->MemoryBytesSize;
		for (FieldSymbol* field : genericInfo->UnderlayingType->Fields)
		{
			TypeSymbol* returnType = field->ReturnType;
			if (returnType == nullptr)
				continue;

			if (returnType->Kind != SyntaxKind::TypeParameter)
				continue;

			returnType = genericInfo->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(returnType));
			objectInfo->MemoryBytesSize += returnType->GetInlineSize();
		}
	}

	objectInfo->State = TypeLayoutingState::Visited;
	return;
}