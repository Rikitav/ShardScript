#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>
#include <shard/parsing/Diagnostics.hpp>

#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/AttributeSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/Directives/NamespaceDirectiveSyntax.hpp>

#include <algorithm>
#include <initializer_list>
#include <memory_resource>
#include <vector>

namespace shard
{
	// Note that this parser is only capable of contextual parsing, and only should be used to parse full compulation units. DO NOT try to parse individual members or expression with this parser out of stream

	class SHARD_API SourceParser
	{
		static constexpr int max_loop_iterations = 10000;

		SyntaxTree& m_syntaxTree;
		DiagnosticsContext& m_diagnostics;

	public:
		SourceParser(SyntaxTree& syntaxTree, DiagnosticsContext& diagnostics);
		~SourceParser() = default;

		void FromSourceProvider(SourceProvider& reader);

	private:
		SyntaxToken expect(SourceProvider& reader, TokenType type, const wchar_t* message);
		bool matches(SourceProvider& reader, std::initializer_list<TokenType> types);
		bool try_match(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips = 5);

		gmt::Ref<TranslationUnitSyntax> read_compilation_unit(SourceProvider& reader);
		gmt::Ref<UsingDirectiveSyntax> read_using_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<NamespaceDirectiveSyntax> read_namespace_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		gmt::Ref<MemberDeclarationSyntax> read_member_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);

		gmt::Ref<AttributeSyntax> read_attribute(SourceProvider& reader, gmt::Ref<SyntaxNode> parent);
		void read_attribute_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, std::pmr::vector<gmt::Ref<AttributeSyntax>>& attributes);
		void read_member_modifiers(SourceProvider& reader, std::pmr::vector<SyntaxToken>& modifiers);
	};
}
