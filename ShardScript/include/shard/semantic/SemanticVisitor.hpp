#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/Diagnostics.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <optional>
#include <stack>
#include <string_view>

namespace shard
{
	class SemanticModel;

	class SHARD_API SemanticVisitor : public SyntaxVisitor
	{
		std::stack<SemanticScope> m_scopeStack;

		SymbolTable* m_table;
		DiagnosticsContext& m_diagnostics;
		SemanticModel* m_model;

	public:
		SemanticVisitor(SemanticModel& model, DiagnosticsContext& diagnostics);
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
		SymbolTable& get_symbol_table();
		DiagnosticsContext& get_diagnostics();
		SemanticModel& get_model();

		template<typename T>
		gmt::Ref<T> lookup_symbol(gmt::Ref<const SyntaxNode> node);

		template<typename T>
		std::optional<gmt::Ref<const T>> ref_from(const T* ptr);

		virtual void declare(gmt::Ref<SyntaxSymbol> symbol);

		virtual bool check_name_declared(std::wstring_view name);
		virtual bool check_name_declared(gmt::Ref<SyntaxSymbol> symbol);

		gmt::Ref<SyntaxSymbol> get_owner_symbol();
		gmt::Ref<StructSymbol> get_owner_type();
		gmt::Ref<MethodSymbol> find_host_method_symbol();

		bool is_symbol_accessible(gmt::Ref<SyntaxSymbol> symbol, gmt::Ref<const SyntaxNode> symbolDeclaringNode, gmt::Ref<const SyntaxNode> callSiteNode);
	};
}

#include <shard/semantic/SemanticVisitor.impl.hpp>
