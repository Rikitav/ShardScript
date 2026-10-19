#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/MethodEffectSummary.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace shard
{
	class StructSymbol;
	class TypeParameterSymbol;
	class ParameterSymbol;

	enum class CallingConvention
	{
		None,		// Unknown or unresolved calling convention. VM treats that as panic and halts immidietly
		Managed,	// Managed bytecode calling convention. VM expects symbol to contain offset of starting opcode for th program
		ShardDecl	// Managed foreighn function calling convention. VM expects CDecl function with signature that accepts MethodCallState argument
	};

	class SHARD_API MethodSymbol : public MemberSymbol
	{
		CallingConvention m_callingConvention = CallingConvention::None;
		void* m_functionPointer = nullptr;

		gmt::Ref<StructSymbol> m_returnType;
		std::vector<gmt::Ref<TypeParameterSymbol>> m_typeParameters;
		std::vector<gmt::Ref<ParameterSymbol>> m_parameters;

		bool m_isVirtual = false;
		bool m_isAsync = false;

		MethodEffectSummary m_effectSummary;
		bool m_effectsComputed = false;

	public:
		MethodSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		MethodSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~MethodSymbol() = default;

		MethodSymbol(const MethodSymbol& other) = delete;
		MethodSymbol& operator=(const MethodSymbol& other) = delete;

		MethodSymbol(MethodSymbol&& other) = delete;
		MethodSymbol& operator=(MethodSymbol&& other) = delete;

		CallingConvention get_calling_convention() const;
		void set_calling_convention(CallingConvention calling_convention);

		void* get_function_pointer() const;
		void set_function_pointer(void* function_pointer);

		gmt::Ref<StructSymbol> get_return_type() const;
		void set_return_type(gmt::Ref<StructSymbol> return_type);

		const std::vector<gmt::Ref<TypeParameterSymbol>>& get_type_parameters() const;
		const std::vector<gmt::Ref<ParameterSymbol>>& get_parameters() const;

		bool get_is_virtual() const;
		void set_is_virtual(bool is_virtual);

		bool get_is_async() const;
		void set_is_async(bool is_async);

		MethodEffectSummary& get_effect_summary();
		const MethodEffectSummary& get_effect_summary() const;

		bool get_effects_computed() const;
		void set_effects_computed(bool effects_computed);

		bool is_method() const override;
		void on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol) override;

	protected:
		MethodSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		MethodSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
	};
}
