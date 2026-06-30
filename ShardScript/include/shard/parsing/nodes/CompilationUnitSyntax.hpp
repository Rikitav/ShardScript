#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	enum CompilationUnitOrigin
	{
		Unknown,
		SourceFile,
		DynamicLib
	};

	class SHARD_API CompilationUnitSyntax : public SyntaxNode
	{
	public:
		CompilationUnitOrigin Origin;
		std::vector<std::unique_ptr<UsingDirectiveSyntax>> Usings;
		std::vector<std::unique_ptr<MemberDeclarationSyntax>> Members;
		std::unique_ptr<NamespaceDeclarationSyntax> Namespace;

		inline CompilationUnitSyntax()
			: SyntaxNode(SyntaxKind::CompilationUnit, nullptr), Origin(CompilationUnitOrigin::Unknown) { }

		inline CompilationUnitSyntax(const CompilationUnitSyntax& other) = delete;

		inline virtual ~CompilationUnitSyntax() = default;
	};
}
