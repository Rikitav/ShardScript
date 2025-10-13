#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>

using namespace std;

namespace shard::syntax::nodes
{
	/*
	enum class MemberAccessModifier : short
	{
		None = 0,
		Public = 1,
		Private = 2,
		Internal = 4,
		Protected = 8,
		Static = 16
	};

	// bit or
	inline MemberAccessModifier operator |(MemberAccessModifier a, MemberAccessModifier b) {
		return static_cast<MemberAccessModifier>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline MemberAccessModifier operator |(int a, MemberAccessModifier b) {
		return static_cast<MemberAccessModifier>(a | static_cast<int>(b));
	}

	// bit and
	inline MemberAccessModifier operator &(MemberAccessModifier a, MemberAccessModifier b) {
		return static_cast<MemberAccessModifier>(static_cast<int>(a) & static_cast<int>(b));
	}

	inline MemberAccessModifier operator &(int a, MemberAccessModifier b) {
		return static_cast<MemberAccessModifier>(a & static_cast<int>(b));
	}

	// has flag
	inline bool operator %(MemberAccessModifier value, MemberAccessModifier flag) {
		using T = std::underlying_type_t<MemberAccessModifier>;
		return (static_cast<T>(value) & static_cast<T>(flag)) == static_cast<T>(flag);
	}

	inline bool operator %(int value, MemberAccessModifier flag) {
		using T = std::underlying_type_t<MemberAccessModifier>;
		return (static_cast<T>(value) & static_cast<T>(flag)) == static_cast<T>(flag);
	}

	// bit or, assign
	inline MemberAccessModifier& operator |=(MemberAccessModifier& a, MemberAccessModifier b) {
		a = a | b;
		return a;
	}

	inline int& operator |=(int& a, MemberAccessModifier b) {
		a = (int)(a | b);
		return a;
	}
	*/

	class MemberDeclarationSyntax : public SyntaxNode
	{
	public:
		vector<SyntaxToken> Modifiers;
		SyntaxToken DeclareKeyword;
		SyntaxToken Identifier;

		MemberDeclarationSyntax()
			: SyntaxNode(SyntaxKind::Unknown), Identifier(), Modifiers() { }

		MemberDeclarationSyntax(SyntaxKind kind)
			: SyntaxNode(kind), Identifier(), Modifiers() { }
	};
}
