#include <shard/parsing/SyntaxNode.hpp>

using namespace shard;

SyntaxNode::SyntaxNode(
	const SyntaxKind kind,
	SyntaxNode* parent
) :
	m_kind(kind),
	m_parent(parent)
{ }

SyntaxNode::~SyntaxNode()
{
	m_parent = nullptr;
}
