#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <string_view>
#include <vector>

namespace shard
{
	class ParameterSymbol;
	class TypeParameterSymbol;

	class SHARD_API DelegateTypeSymbol final : public StructSymbol
	{
		StructSymbol* m_returnType = nullptr;
		gmt::vector<ParameterSymbol*> m_parameters;
		bool m_isAsync = false;

	public:
		DelegateTypeSymbol(std::wstring_view name, SyntaxSymbol* parent);
		DelegateTypeSymbol(gmt::StringReference name, SyntaxSymbol* parent);
		virtual ~DelegateTypeSymbol() = default;

		DelegateTypeSymbol(const DelegateTypeSymbol& other) = delete;
		DelegateTypeSymbol& operator=(const DelegateTypeSymbol& other) = delete;

		DelegateTypeSymbol(DelegateTypeSymbol&& other) = delete;
		DelegateTypeSymbol& operator=(DelegateTypeSymbol&& other) = delete;

		StructSymbol* get_return_type() const;
		void set_return_type(StructSymbol* return_type);

		const gmt::vector<ParameterSymbol*>& get_parameters() const;
		void set_parameters(const gmt::vector<ParameterSymbol*>& parameters);

		const gmt::vector<TypeParameterSymbol*>& get_type_parameters() const;
		void set_type_parameters(const gmt::vector<TypeParameterSymbol*>& type_parameters);

		bool get_is_async() const;
		void set_is_async(bool is_async);
	};
}
