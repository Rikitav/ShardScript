#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <vector>

namespace shard
{
	class SHARD_API AttributeSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenBracketToken;
		SyntaxToken CloseBracketToken;
		SyntaxToken NameToken;
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		std::vector<SyntaxToken> Arguments;

		inline AttributeSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::Attribute, parent) { }

		inline AttributeSyntax(const AttributeSyntax&) = delete;
		inline virtual ~AttributeSyntax() { }
	};
}
