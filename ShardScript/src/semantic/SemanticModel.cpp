#include <shard/semantic/SemanticModel.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Containers.hpp>

#include <stdexcept>
#include <unordered_set>

using namespace shard;

SemanticModel::SemanticModel(shard::SyntaxTree& tree)
	: m_tree(tree) { }

SyntaxTree& SemanticModel::get_tree() const
{
	return m_tree;
}

SymbolTable& SemanticModel::get_table()
{
	return m_table;
}

StructSymbol* SemanticModel::get_expression_type(const ExpressionSyntax* expression) const
{
	if (expression == nullptr)
		return nullptr;

	auto it = m_expressionTypes.find(expression);
	return it != m_expressionTypes.end() ? it->second : nullptr;
}

void SemanticModel::set_expression_type(const ExpressionSyntax* expression, StructSymbol* type)
{
	if (!(expression == nullptr))
		m_expressionTypes[expression] = type;
}

bool SemanticModel::are_types_equal(const StructSymbol* left, const StructSymbol* right)
{
	if ((left == nullptr) || (right == nullptr))
		return false;

	switch (left->get_kind())
	{
		default:
			return left == right;

		case SyntaxKind::GenericType:
		{
			if (right->get_kind() != SyntaxKind::GenericType)
				return false;

			const GenericTypeSymbol* leftGeneric = gmt::as<GenericTypeSymbol>(left);
			const GenericTypeSymbol* rightGeneric = gmt::as<GenericTypeSymbol>(right);

			const StructSymbol* leftUnderlaying = leftGeneric->get_underlaying_type();
			const StructSymbol* rightUnderlaying = rightGeneric->get_underlaying_type();

			if (!are_types_equal(leftUnderlaying, rightUnderlaying))
				return false;

			std::size_t size = leftUnderlaying->type_parameters.size();
			if (rightUnderlaying->type_parameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				StructSymbol* leftArgument = leftGeneric->substitute_type_parameter(leftUnderlaying->type_parameters[i]);
				StructSymbol* rightArgument = rightGeneric->substitute_type_parameter(rightUnderlaying->type_parameters[i]);

				if (!are_types_equal(leftArgument, rightArgument))
					return false;
			}

			return true;
		}

		case SyntaxKind::DelegateType:
		{
			if (right->get_kind() != SyntaxKind::DelegateType)
				return false;

			const DelegateTypeSymbol* leftDelegate = gmt::as<DelegateTypeSymbol>(left);
			const DelegateTypeSymbol* rightDelegate = gmt::as<DelegateTypeSymbol>(right);

			if ((leftDelegate->get_return_type() == nullptr) || (rightDelegate->get_return_type() == nullptr))
				return false;

			if (!are_types_equal(leftDelegate->get_return_type(), rightDelegate->get_return_type()))
				return false;

			std::size_t size = leftDelegate->get_parameters().size();
			if (rightDelegate->get_parameters().size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				if (!are_types_equal(leftDelegate->get_parameters()[i]->get_type(), rightDelegate->get_parameters()[i]->get_type()))
					return false;
			}

			return true;
		}

		case SyntaxKind::ArrayType:
		{
			if (right->get_kind() != SyntaxKind::ArrayType)
				return false;

			const ArrayTypeSymbol* leftArray = gmt::as<ArrayTypeSymbol>(left);
			const ArrayTypeSymbol* rightArray = gmt::as<ArrayTypeSymbol>(right);

			if ((leftArray->get_underlaying_type() == nullptr) || (rightArray->get_underlaying_type() == nullptr))
				return false;

			return are_types_equal(leftArray->get_underlaying_type(), rightArray->get_underlaying_type());
		}

		case SyntaxKind::NullableType:
		{
			if (right->get_kind() != SyntaxKind::NullableType)
				return false;

			return are_types_equal(gmt::as<NullableTypeSymbol>(left)->get_underlaying_type(), gmt::as<NullableTypeSymbol>(right)->get_underlaying_type());
		}
	}
}

static void collect_all_interfaces(const StructSymbol* type, gmt::vector<const StructSymbol*>& out, gmt::unordered_set<const StructSymbol*>& visited)
{
	if (type == nullptr)
		return;

	for (StructSymbol* interface : type->interfaces)
	{
		if (interface == nullptr)
			continue;

		const StructSymbol* constInterface = interface;
		if (visited.insert(constInterface).second)
		{
			out.push_back(constInterface);
			collect_all_interfaces(constInterface, out, visited);
		}
	}
}

static gmt::vector<const StructSymbol*> get_all_interfaces(const StructSymbol* type)
{
	gmt::vector<const StructSymbol*> result;
	gmt::unordered_set<const StructSymbol*> visited;
	collect_all_interfaces(type, result, visited);
	return result;
}

static bool is_constructed_interface_implemented(GenericTypeSymbol* targetInterface, const StructSymbol* sourceType)
{
	StructSymbol* underlaying = targetInterface->get_underlaying_type();
	if (underlaying->get_kind() != SyntaxKind::InterfaceDeclaration)
		return false;

	auto matchesInterface = [&](GenericTypeSymbol* candidateInterface) -> bool
	{
		if (candidateInterface->get_underlaying_type() != underlaying)
			return false;

		for (TypeParameterSymbol* parameter : underlaying->type_parameters)
		{
			StructSymbol* targetArgument = targetInterface->substitute_type_parameter(parameter);
			StructSymbol* sourceArgument = candidateInterface->substitute_type_parameter(parameter);

			if (!(sourceArgument == nullptr) && sourceArgument->get_kind() == SyntaxKind::TypeParameter && !(sourceType == nullptr) && sourceType->get_kind() == SyntaxKind::GenericType)
				sourceArgument = gmt::as<GenericTypeSymbol>(sourceType)->substitute_type_parameter(static_cast<TypeParameterSymbol*>(sourceArgument));

			if (!SemanticModel::are_types_equal(targetArgument, sourceArgument))
				return false;
		}

		return true;
	};

	auto checkTypeInterfaces = [&](const StructSymbol* type) -> bool
	{
		for (const StructSymbol* interface : get_all_interfaces(type))
		{
			if (interface->get_kind() != SyntaxKind::GenericType)
				continue;

			if (matchesInterface(gmt::as<GenericTypeSymbol>(interface)))
				return true;
		}

		return false;
	};

	if (checkTypeInterfaces(sourceType))
		return true;

	if (sourceType->get_kind() == SyntaxKind::GenericType)
	{
		if (checkTypeInterfaces(gmt::as<GenericTypeSymbol>(sourceType)->get_underlaying_type()))
			return true;
	}

	return false;
}

bool SemanticModel::is_assignable_to(const StructSymbol* target, const StructSymbol* source) const
{
	if ((target == nullptr) || (source == nullptr))
		return false;

	if (are_types_equal(target, source))
		return true;

	// TODO: pragmatic analysis — flow-based null-state checks
	if (source->get_kind() == SyntaxKind::NullableType && target->get_kind() != SyntaxKind::NullableType)
		return is_assignable_to(target, gmt::as<NullableTypeSymbol>(source)->get_underlaying_type());

	if (source->get_kind() != SyntaxKind::NullableType && target->get_kind() == SyntaxKind::NullableType)
		return is_assignable_to(gmt::as<NullableTypeSymbol>(target)->get_underlaying_type(), source);

	// numeric widening: byte/char/nint convert to int, anything numeric to double
	bool sourceNumeric = source == m_table.get_double_primitive();
	if (!sourceNumeric)
		sourceNumeric = source == m_table.get_integer_primitive() || source == m_table.get_char_primitive();
	if (!sourceNumeric)
		sourceNumeric = source == m_table.get_byte_primitive() || source == m_table.get_nint_primitive();

	if (target == m_table.get_double_primitive() && sourceNumeric)
		return true;

	bool sourceSmallerThanInt = source == m_table.get_char_primitive() || source == m_table.get_byte_primitive();
	if (!sourceSmallerThanInt)
		sourceSmallerThanInt = source == m_table.get_nint_primitive();

	if (target == m_table.get_integer_primitive() && sourceSmallerThanInt)
		return true;

	if (target->get_kind() == SyntaxKind::InterfaceDeclaration)
	{
		for (const StructSymbol* interface : get_all_interfaces(source))
		{
			if (are_types_equal(target, interface))
				return true;
		}

		if (source->get_kind() == SyntaxKind::ArrayType)
		{
			for (const StructSymbol* interface : get_all_interfaces(m_table.get_array_primitive()))
			{
				if (are_types_equal(target, interface))
					return true;
			}
		}
	}

	if (target->get_kind() == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* targetGeneric = gmt::as<GenericTypeSymbol>(target);
		if (is_constructed_interface_implemented(targetGeneric, source))
			return true;

		if (targetGeneric->get_underlaying_type()->get_kind() == SyntaxKind::DelegateType && source->get_kind() == SyntaxKind::DelegateType)
		{
			const DelegateTypeSymbol* targetDelegate = static_cast<DelegateTypeSymbol*>(targetGeneric->get_underlaying_type());
			const DelegateTypeSymbol* sourceDelegate = gmt::as<DelegateTypeSymbol>(source);

			if ((targetDelegate->get_return_type() == nullptr) || (sourceDelegate->get_return_type() == nullptr))
				return false;

			StructSymbol* targetReturn = targetDelegate->get_return_type();
			if (targetReturn->get_kind() == SyntaxKind::TypeParameter)
			{
				StructSymbol* substituted = targetGeneric->substitute_type_parameter(static_cast<TypeParameterSymbol*>(targetReturn));
				if (!(substituted == nullptr))
					targetReturn = substituted;
			}

			if (!are_types_equal(targetReturn, sourceDelegate->get_return_type()))
				return false;

			if (targetDelegate->get_parameters().size() != sourceDelegate->get_parameters().size())
				return false;

			for (std::size_t i = 0; i < targetDelegate->get_parameters().size(); ++i)
			{
				StructSymbol* targetParameter = targetDelegate->get_parameters()[i]->get_type();
				if (targetParameter->get_kind() == SyntaxKind::TypeParameter)
				{
					StructSymbol* substituted = targetGeneric->substitute_type_parameter(static_cast<TypeParameterSymbol*>(targetParameter));
					if (!(substituted == nullptr))
						targetParameter = substituted;
				}

				if (!are_types_equal(targetParameter, sourceDelegate->get_parameters()[i]->get_type()))
					return false;
			}

			return true;
		}
	}

	if (target->get_kind() == SyntaxKind::DelegateType && source->get_kind() == SyntaxKind::DelegateType)
	{
		const DelegateTypeSymbol* targetDelegate = gmt::as<DelegateTypeSymbol>(target);
		const DelegateTypeSymbol* sourceDelegate = gmt::as<DelegateTypeSymbol>(source);

		if (!are_types_equal(targetDelegate->get_return_type(), sourceDelegate->get_return_type()))
			return false;

		if (targetDelegate->get_parameters().size() != sourceDelegate->get_parameters().size())
			return false;

		for (std::size_t i = 0; i < targetDelegate->get_parameters().size(); ++i)
		{
			if (!are_types_equal(targetDelegate->get_parameters()[i]->get_type(), sourceDelegate->get_parameters()[i]->get_type()))
				return false;
		}

		return true;
	}

	return false;
}

gmt::wstring SemanticModel::get_type_display_name(const StructSymbol* type) const
{
	if (type == nullptr)
		return L"<error>";

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		const GenericTypeSymbol* generic = gmt::as<GenericTypeSymbol>(type);
		const StructSymbol* underlaying = generic->get_underlaying_type();

		gmt::wstring result = (underlaying == nullptr) ? L"<error>" : gmt::wstring(underlaying->get_name());
		result += L'<';

		if (!(underlaying == nullptr))
		{
			bool first = true;
			for (TypeParameterSymbol* parameter : underlaying->type_parameters)
			{
				if (!first)
					result += L", ";

				StructSymbol* argument = generic->substitute_type_parameter(parameter);
				result += get_type_display_name((argument == nullptr) ? static_cast<const StructSymbol*>(parameter) : argument);
				first = false;
			}
		}

		result += L'>';
		return result;
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = gmt::as<ArrayTypeSymbol>(type);
		return gmt::concat(get_type_display_name(array->get_underlaying_type()), L"[]");
	}

	if (type->get_kind() == SyntaxKind::NullableType)
	{
		const NullableTypeSymbol* nullable = gmt::as<NullableTypeSymbol>(type);
		return gmt::concat(get_type_display_name(nullable->get_underlaying_type()), L"?");
	}

	return gmt::wstring(type->get_name());
}

bool SemanticModel::type_implements_interface(StructSymbol* type, InterfaceSymbol* interfaceSymbol) const
{
	if ((type == nullptr) || (interfaceSymbol == nullptr))
		return false;

	for (StructSymbol* const& interfaceType : type->interfaces)
	{
		if (interfaceType == nullptr)
			continue;

		if (interfaceType == interfaceSymbol)
			return true;

		bool constructedMatch = interfaceType->get_kind() == SyntaxKind::GenericType;
		if (constructedMatch && static_cast<GenericTypeSymbol*>(interfaceType)->get_underlaying_type() == interfaceSymbol)
			return true;
	}

	return false;
}

bool SemanticModel::type_satisfies_constraint(StructSymbol* typeArgument, StructSymbol* constraint) const
{
	if ((typeArgument == nullptr) || (constraint == nullptr))
		return false;

	if (constraint->get_kind() == SyntaxKind::InterfaceDeclaration)
		return type_implements_interface(typeArgument, static_cast<InterfaceSymbol*>(constraint));

	if (constraint->is_type())
		return is_assignable_to(constraint, typeArgument);

	return false;
}

bool SemanticModel::is_primitive_type(const StructSymbol* type) const
{
	return type == m_table.get_boolean_primitive()
		|| type == m_table.get_integer_primitive()
		|| type == m_table.get_double_primitive()
		|| type == m_table.get_char_primitive()
		|| type == m_table.get_string_primitive()
		|| type == m_table.get_array_primitive()
		|| type == m_table.get_nint_primitive();
}

bool SemanticModel::is_task_or_task_like(StructSymbol* type)
{
	if (type == nullptr)
		return false;

	if (type == m_table.get_standard_type(StandardTypes::Task))
		return true;

	if (type->get_kind() != SyntaxKind::GenericType)
		return false;

	StructSymbol* underlaying = static_cast<GenericTypeSymbol*>(type)->get_underlaying_type();
	return underlaying == m_table.get_standard_type(StandardTypes::ValueTask);
}

StructSymbol* SemanticModel::substitute_type(StructSymbol* type, GenericTypeSymbol* within)
{
	if (type == nullptr)
		return nullptr;

	switch (type->get_kind())
	{
		default:
			throw std::runtime_error("unknown syntax kind to substitute");

		case SyntaxKind::StructDeclaration:
		case SyntaxKind::InterfaceDeclaration:
		case SyntaxKind::EnumDeclaration:
		case SyntaxKind::DelegateType:
			return type;

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
			StructSymbol* underlayingType = arrayType->get_underlaying_type();

			if (underlayingType == nullptr)
				throw std::runtime_error("cannot resolve underlaying type of array");

			if (underlayingType->get_kind() == SyntaxKind::TypeParameter)
			{
				if (within == nullptr)
					return type;

				underlayingType = within->substitute_type_parameter(static_cast<TypeParameterSymbol*>(underlayingType));
				if (underlayingType == nullptr)
					throw std::runtime_error("cannot resolve underlaying type of array");

				return m_table.find_or_create_array_type(underlayingType, arrayType->get_length());
			}

			return arrayType;
		}

		case SyntaxKind::TypeParameter:
		{
			if (within == nullptr)
				return type;

			StructSymbol* resolvedType = within->substitute_type_parameter(static_cast<TypeParameterSymbol*>(type));
			return !(resolvedType == nullptr) ? resolvedType : type;
		}

		case SyntaxKind::NullableType:
		{
			NullableTypeSymbol* nullable = static_cast<NullableTypeSymbol*>(type);
			StructSymbol* underlaying = substitute_type(nullable->get_underlaying_type(), within);

			if ((underlaying == nullptr) || underlaying == nullable->get_underlaying_type())
				return type;

			return m_table.find_or_create_nullable_type(underlaying);
		}
	}
}

StructSymbol* SemanticModel::get_field_type(FieldSymbol* field, GenericTypeSymbol* within)
{
	if ((field == nullptr) || (field->get_return_type() == nullptr))
		return nullptr;

	return substitute_type(field->get_return_type(), within);
}

StructSymbol* SemanticModel::get_method_return_type(MethodSymbol* method, GenericTypeSymbol* within)
{
	if ((method == nullptr) || (method->get_return_type() == nullptr))
		return nullptr;

	return substitute_type(method->get_return_type(), within);
}

StructSymbol* SemanticModel::get_property_type(PropertySymbol* property, GenericTypeSymbol* within)
{
	if ((property == nullptr) || (property->get_return_type() == nullptr))
		return nullptr;

	return substitute_type(property->get_return_type(), within);
}

StructSymbol* SemanticModel::get_constructor_return_type(ConstructorSymbol* constructor, GenericTypeSymbol* within)
{
	if ((constructor == nullptr) || (constructor->get_return_type() == nullptr))
		return nullptr;

	return substitute_type(constructor->get_return_type(), within);
}

static StructSymbol* find_type_in_namespace(NamespaceSymbol* scope, std::wstring_view name)
{
	if (scope == nullptr)
		return nullptr;

	for (SyntaxSymbol* member : scope->members)
	{
		if (!(member == nullptr) && member->is_type() && member->get_name() == name)
			return static_cast<StructSymbol*>(member);
	}

	for (const auto& child : scope->get_children())
	{
		StructSymbol* result = find_type_in_namespace(child.second, name);
		if (!(result == nullptr))
			return result;
	}

	return nullptr;
}

StructSymbol* SemanticModel::find_type_by_name(std::wstring_view fullName)
{
	StructSymbol* primitives[] = {
		m_table.get_void_primitive(),
		m_table.get_boolean_primitive(),
		m_table.get_integer_primitive(),
		m_table.get_double_primitive(),
		m_table.get_char_primitive(),
		m_table.get_string_primitive(),
		m_table.get_array_primitive(),
		m_table.get_nint_primitive(),
		m_table.get_byte_primitive(),
	};

	std::size_t dot = fullName.find(L'.');
	if (dot == std::wstring_view::npos)
	{
		for (StructSymbol* primitive : primitives)
		{
			if (!(primitive == nullptr) && primitive->get_name() == fullName)
				return primitive;
		}

		return find_type_in_namespace(m_table.get_root_namespace(), fullName);
	}

	NamespaceSymbol* node = m_table.get_root_namespace();
	std::size_t start = 0;
	std::size_t end = fullName.find(L'.');

	while (!(node == nullptr) && end != std::wstring_view::npos)
	{
		node = node->lookup(fullName.substr(start, end - start));
		start = end + 1;
		end = fullName.find(L'.', start);
	}

	if (!(node == nullptr))
	{
		std::wstring_view typeName = fullName.substr(start);
		for (SyntaxSymbol* member : node->members)
		{
			if ((member == nullptr) || !member->is_type() || member->get_name() != typeName)
				continue;

			return static_cast<StructSymbol*>(member);
		}
	}

	return nullptr;
}

FieldSymbol* SemanticModel::find_field_by_name(StructSymbol* type, std::wstring_view name) const
{
	if (type == nullptr)
		return nullptr;

	for (FieldSymbol* field : type->fields)
	{
		if (field->get_name() == name)
			return field;
	}

	return nullptr;
}

bool SemanticModel::resolve_indexable(StructSymbol* type, PropertySymbol*& outCount, IndexatorSymbol*& outIndexator)
{
	outCount = nullptr;
	outIndexator = nullptr;

	if (type == nullptr)
		return false;

	StructSymbol* indexableTrait = static_cast<StructSymbol*>(m_table.get_standard_type(StandardTypes::IIndexable));
	if (indexableTrait == nullptr)
		return false;

	StructSymbol* searchType = type;
	GenericTypeSymbol* within = nullptr;
	if (searchType->get_kind() == SyntaxKind::GenericType)
	{
		within = static_cast<GenericTypeSymbol*>(searchType);
		searchType = within->get_underlaying_type();
	}

	// structural conformance: every abstract interface member must resolve to
	// a concrete member on the type, explicitly implemented or not
	for (PropertySymbol* property : indexableTrait->properties)
	{
		outCount = resolve_virtual_property(searchType, property, within);
		if (outCount == nullptr)
			return false;
	}

	for (IndexatorSymbol* indexator : indexableTrait->indexators)
	{
		outIndexator = resolve_virtual_indexator(searchType, indexator, within);
		if (outIndexator == nullptr)
			return false;
	}

	return true;
}

PropertySymbol* SemanticModel::resolve_virtual_property(StructSymbol* type, PropertySymbol* interfaceMember, GenericTypeSymbol* within)
{
	if ((type == nullptr) || (interfaceMember == nullptr))
		return nullptr;

	StructSymbol* expectedType = substitute_type(interfaceMember->get_return_type(), within);
	for (PropertySymbol* candidate : type->properties)
	{
		if (candidate->get_name() != interfaceMember->get_name())
			continue;

		bool getterOk = (interfaceMember->get_getter() == nullptr) || !(candidate->get_getter() == nullptr);
		bool setterOk = (interfaceMember->get_setter() == nullptr) || !(candidate->get_setter() == nullptr);
		if (!getterOk || !setterOk)
			continue;

		StructSymbol* candidateType = substitute_type(candidate->get_return_type(), within);
		if (are_types_equal(expectedType, candidateType))
			return candidate;
	}

	return nullptr;
}

IndexatorSymbol* SemanticModel::resolve_virtual_indexator(StructSymbol* type, IndexatorSymbol* interfaceMember, GenericTypeSymbol* within)
{
	if ((type == nullptr) || (interfaceMember == nullptr))
		return nullptr;

	const gmt::vector<ParameterSymbol*>& expectedParams = interfaceMember->get_parameters();
	for (IndexatorSymbol* candidate : type->indexators)
	{
		const gmt::vector<ParameterSymbol*>& candidateParams = candidate->get_parameters();
		if (candidateParams.size() != expectedParams.size())
			continue;

		bool getterOk = (interfaceMember->get_getter() == nullptr) || !(candidate->get_getter() == nullptr);
		bool setterOk = (interfaceMember->get_setter() == nullptr) || !(candidate->get_setter() == nullptr);
		if (!getterOk || !setterOk)
			continue;

		bool paramsMatch = true;
		for (std::size_t i = 0; i < expectedParams.size(); ++i)
		{
			StructSymbol* candidateType = substitute_type(candidateParams[i]->get_type(), within);
			if (!are_types_equal(expectedParams[i]->get_type(), candidateType))
			{
				paramsMatch = false;
				break;
			}
		}

		if (paramsMatch)
			return candidate;
	}

	return nullptr;
}

StructSymbol* SemanticModel::resolve_runtime_type_argument(StructSymbol* type, const TypeParameterResolver& resolveParameter)
{
	if (type == nullptr)
		return type;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		StructSymbol* resolved = resolveParameter(static_cast<TypeParameterSymbol*>(type));
		return !(resolved == nullptr) ? resolved : type;
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
		StructSymbol* resolvedUnderlaying = resolve_runtime_type_argument(arrayType->get_underlaying_type(), resolveParameter);

		if ((resolvedUnderlaying == nullptr) || resolvedUnderlaying == arrayType->get_underlaying_type())
			return type;

		return m_table.find_or_create_array_type(resolvedUnderlaying, arrayType->get_length());
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		StructSymbol* baseType = nullptr;
		gmt::vector<StructSymbol*> genericArgs;

		if (!try_resolve_generic_arguments(type, resolveParameter, baseType, genericArgs))
			return type;

		bool changed = false;
		GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
		const gmt::vector<TypeParameterSymbol*>& typeParameters = baseType->type_parameters;
		for (std::size_t i = 0; i < typeParameters.size() && i < genericArgs.size(); ++i)
		{
			if (genericArgs[i] != generic->substitute_type_parameter(typeParameters[i]))
			{
				changed = true;
				break;
			}
		}

		if (!changed)
			return type;

		return m_table.find_or_create_generic_type(baseType, genericArgs);
	}

	if (type->get_kind() == SyntaxKind::NullableType)
	{
		NullableTypeSymbol* nullable = static_cast<NullableTypeSymbol*>(type);
		StructSymbol* resolvedUnderlaying = resolve_runtime_type_argument(nullable->get_underlaying_type(), resolveParameter);

		if ((resolvedUnderlaying == nullptr) || resolvedUnderlaying == nullable->get_underlaying_type())
			return type;

		return m_table.find_or_create_nullable_type(resolvedUnderlaying);
	}

	return type;
}

StructSymbol* SemanticModel::substitute_type(StructSymbol* type, std::span<StructSymbol* const> typeArguments)
{
	if ((type == nullptr) || typeArguments.empty())
		return type;

	const TypeParameterResolver resolver = [typeArguments](TypeParameterSymbol* parameter) -> StructSymbol*
	{
		const std::uint16_t index = parameter->get_type_argument_index();
		if (index >= typeArguments.size())
			return nullptr;

		return typeArguments[index];
	};

	return resolve_runtime_type_argument(type, resolver);
}

bool SemanticModel::try_resolve_generic_arguments(StructSymbol* type, const TypeParameterResolver& resolveParameter, StructSymbol*& baseType, gmt::vector<StructSymbol*>& genericArgs)
{
	if (type == nullptr)
		return false;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		StructSymbol* resolved = resolveParameter(static_cast<TypeParameterSymbol*>(type));
		if (!(resolved == nullptr) && resolved != type)
			return try_resolve_generic_arguments(resolved, resolveParameter, baseType, genericArgs);

		return false;
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
		baseType = generic->get_underlaying_type();

		genericArgs.clear();
		genericArgs.reserve(baseType->type_parameters.size());
		for (TypeParameterSymbol* parameter : baseType->type_parameters)
		{
			StructSymbol* argument = generic->substitute_type_parameter(parameter);
			StructSymbol* resolvedArgument = resolve_runtime_type_argument(argument, resolveParameter);
			genericArgs.push_back(!(resolvedArgument == nullptr) ? resolvedArgument : argument);
		}

		return true;
	}

	return false;
}
