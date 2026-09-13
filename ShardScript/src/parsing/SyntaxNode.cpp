#include <shard/parsing/SyntaxNode.hpp>
#include <gmt/Arena.hpp>

using namespace shard;

SyntaxNode::SyntaxNode(
	const SyntaxKind kind,
	gmt::Ref<SyntaxNode> parent
) :
	m_kind(kind),
	m_parent(parent)
{ }

SyntaxNode::~SyntaxNode()
{
	m_parent = gmt::Ref<SyntaxNode>();
}
