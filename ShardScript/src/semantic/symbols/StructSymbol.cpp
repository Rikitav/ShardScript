#include <shard/semantic/symbols/StructSymbol.hpp>

#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

StructSymbol::StructSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, kind, parent) { }

StructSymbol::StructSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, kind, parent) { }

bool StructSymbol::is_type() const
{
	return true;
}

void StructSymbol::on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol)
{
	switch (symbol->get_kind())
	{
		case SyntaxKind::ConstructorDeclaration:
		{
			constructors.push_back(symbol.as<ConstructorSymbol>());
			break;
		}

		case SyntaxKind::FunctionDeclaration:
		{
			methods.push_back(symbol.as<MethodSymbol>());
			break;
		}

		case SyntaxKind::OperatorDeclaration:
		{
			operators.push_back(symbol.as<OperatorSymbol>());
			break;
		}

		case SyntaxKind::FieldDeclaration:
		{
			fields.push_back(symbol.as<FieldSymbol>());
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			properties.push_back(symbol.as<PropertySymbol>());
			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			indexators.push_back(symbol.as<IndexatorSymbol>());
			break;
		}

		case SyntaxKind::TypeParameter:
		{
			type_parameters.push_back(symbol.as<TypeParameterSymbol>());
			break;
		}
	}
}
