#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <stack>
#include <string_view>

namespace shard
{
	class SHARD_API SemanticVisitor
	{
		std::stack<SemanticScope> m_scopeStack;

	public:
		SemanticVisitor() = default;
		virtual ~SemanticVisitor();

		SemanticVisitor(const SemanticVisitor& other) = delete;
		SemanticVisitor& operator=(const SemanticVisitor& other) = delete;

		SemanticVisitor(SemanticVisitor&& other) = delete;
		SemanticVisitor& operator=(SemanticVisitor&& other) = delete;

		SemanticScope* get_current_scope();
		void pop_scope_stack();
		void push_scope(gmt::Ref<SyntaxSymbol> symbol);
		void pop_scope();

	protected:
		virtual void declare(gmt::Ref<SyntaxSymbol> symbol);

		virtual bool check_name_declared(std::wstring_view name);
		virtual bool check_name_declared(gmt::Ref<SyntaxSymbol> symbol);

		gmt::Ref<SyntaxSymbol> get_owner_symbol();
		gmt::Ref<StructSymbol> get_owner_type();
		gmt::Ref<MethodSymbol> find_host_method_symbol();

		bool is_symbol_accessible(gmt::Ref<SyntaxSymbol> symbol, gmt::Ref<const SyntaxNode> symbolDeclaringNode, gmt::Ref<const SyntaxNode> callSiteNode);
	};
}
