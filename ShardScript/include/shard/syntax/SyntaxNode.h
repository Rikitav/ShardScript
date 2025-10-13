#pragma once
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/SyntaxKind.h>
#include <memory>

using namespace std;
using namespace shard::syntax::analysis;

namespace shard::syntax
{
	class SyntaxNode
	{
	public:
		shared_ptr<SyntaxNode> Parent;
		SyntaxKind Kind;
		//bool IsMissing;

		SyntaxNode(SyntaxKind kind)
			: Kind(kind) {}

		virtual ~SyntaxNode()
		{

		}
	};
}
