#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <vector>

namespace shard
{
	class SHARD_API AttributeSyntax final : public SyntaxNode
	{
		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_nameToken;
		SyntaxToken m_openCurlToken;
		SyntaxToken m_closeCurlToken;
		std::vector<SyntaxToken> m_arguments;

	public:
		AttributeSyntax(SyntaxNode* parent);
		virtual ~AttributeSyntax() = default;

		virtual TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
