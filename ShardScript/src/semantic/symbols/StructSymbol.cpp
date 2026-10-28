#include <shard/semantic/symbols/StructSymbol.hpp>

#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

StructSymbol::StructSymbol(std::wstring_view name, const SyntaxKind kind, SyntaxSymbol* parent)
	: MemberSymbol(name, kind, parent) { }

StructSymbol::StructSymbol(gmt::StringReference name, const SyntaxKind kind, SyntaxSymbol* parent)
	: MemberSymbol(name, kind, parent) { }

bool StructSymbol::is_type() const
{
	return true;
}

void StructSymbol::on_symbol_declared(SyntaxSymbol* symbol)
{
	switch (symbol->get_kind())
	{
		case SyntaxKind::ConstructorDeclaration:
		{
			constructors.push_back(static_cast<ConstructorSymbol*>(symbol));
			break;
		}

		case SyntaxKind::FunctionDeclaration:
		{
			methods.push_back(static_cast<MethodSymbol*>(symbol));
			break;
		}

		case SyntaxKind::OperatorDeclaration:
		{
			operators.push_back(static_cast<OperatorSymbol*>(symbol));
			break;
		}

		case SyntaxKind::FieldDeclaration:
		{
			fields.push_back(static_cast<FieldSymbol*>(symbol));
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			properties.push_back(static_cast<PropertySymbol*>(symbol));
			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			indexators.push_back(static_cast<IndexatorSymbol*>(symbol));
			break;
		}

		case SyntaxKind::TypeParameter:
		{
			type_parameters.push_back(static_cast<TypeParameterSymbol*>(symbol));
			break;
		}

		default:
			break;
	}
}