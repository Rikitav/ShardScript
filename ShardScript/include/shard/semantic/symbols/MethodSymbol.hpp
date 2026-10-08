#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <string_view>
#include <vector>

namespace shard
{
	class StructSymbol;
	class TypeParameterSymbol;
	class ParameterSymbol;

	class SHARD_API MethodSymbol : public MemberSymbol
	{
		gmt::Ref<StructSymbol> m_returnType;
		std::vector<gmt::Ref<TypeParameterSymbol>> m_typeParameters;
		std::vector<gmt::Ref<ParameterSymbol>> m_parameters;
		bool m_isAbstract = false;
		bool m_isAsync = false;

	public:
		MethodSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		MethodSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~MethodSymbol() = default;

		MethodSymbol(const MethodSymbol& other) = delete;
		MethodSymbol& operator=(const MethodSymbol& other) = delete;

		MethodSymbol(MethodSymbol&& other) = delete;
		MethodSymbol& operator=(MethodSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_return_type() const;
		void set_return_type(gmt::Ref<StructSymbol> return_type);

		const std::vector<gmt::Ref<TypeParameterSymbol>>& get_type_parameters() const;
		const std::vector<gmt::Ref<ParameterSymbol>>& get_parameters() const;

		bool get_is_abstract() const;
		void set_is_abstract(bool is_abstract);

		bool get_is_async() const;
		void set_is_async(bool is_async);

		bool is_method() const override;
		void on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol) override;

	protected:
		MethodSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		MethodSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
	};
}
