#include <shard/compilation/LayoutGenerator.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

void LayoutGenerator::Generate(SemanticModel& semanticModel)
{
	for (TypeSymbol* objectInfo : semanticModel.Table->GetTypeSymbols())
	{
		if (objectInfo->LayoutingState == TypeLayoutingState::Visited)
			continue;

		FixObjectLayout(semanticModel, objectInfo);
	}
}

void LayoutGenerator::FixObjectLayout(SemanticModel& semanticModel, TypeSymbol* objectInfo)
{
	objectInfo->LayoutingState = TypeLayoutingState::Visiting;

	for (FieldSymbol* field : objectInfo->Fields)
	{
		if (field->Linking == LINK_STATIC)
			continue;

		TypeSymbol* returnType = field->ReturnType;
		if (returnType == nullptr)
			continue;

		if (returnType->Inlining == TypeInlining::ByValue)
		{
			if (returnType->LayoutingState == TypeLayoutingState::Visiting)
			{
				SyntaxToken token = static_cast<FieldDeclarationSyntax*>(semanticModel.Table->LookupNode(field).value_or(nullptr))->IdentifierToken;
				Diagnostics.ReportError(token, L"Recursive struct inlining");
				continue;
			}
		}

		if (returnType->LayoutingState == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, returnType);

		if (field->Linking == LINK_INSTANCE)
		{
			field->MemoryBytesOffset = objectInfo->MemoryBytesSize;
			objectInfo->MemoryBytesSize += returnType->GetInlineSize();
		}
	}

	if (objectInfo->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayInfo = static_cast<ArrayTypeSymbol*>(objectInfo);
		if (arrayInfo->UnderlayingType->LayoutingState == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, arrayInfo->UnderlayingType);

		objectInfo->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->MemoryBytesSize * arrayInfo->Length;
	}

	if (objectInfo->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericInfo = static_cast<GenericTypeSymbol*>(objectInfo);
		if (genericInfo->UnderlayingType->LayoutingState == TypeLayoutingState::Unvisited)
			FixObjectLayout(semanticModel, genericInfo->UnderlayingType);

		genericInfo->FieldOffsets.clear();
		std::size_t offset = 0;
		for (FieldSymbol* field : genericInfo->UnderlayingType->Fields)
		{
			if (field->Linking == LINK_STATIC)
				continue;

			TypeSymbol* fieldType = field->ReturnType;
			if (fieldType == nullptr)
				continue;

			if (fieldType->Kind == SyntaxKind::TypeParameter)
			{
				TypeSymbol* concreteType = genericInfo->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(fieldType));
				if (concreteType != nullptr)
					fieldType = concreteType;
			}

			if (fieldType->LayoutingState == TypeLayoutingState::Unvisited)
				FixObjectLayout(semanticModel, fieldType);

			genericInfo->FieldOffsets[field] = offset;
			offset += fieldType->GetInlineSize();
		}

		objectInfo->MemoryBytesSize = offset;
	}

	objectInfo->LayoutingState = TypeLayoutingState::Visited;
	return;
}