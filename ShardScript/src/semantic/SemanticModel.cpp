#include <shard/semantic/SemanticModel.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Concat.hpp>

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

gmt::Ref<StructSymbol> SemanticModel::get_expression_type(gmt::Ref<const ExpressionSyntax> expression) const
{
	if (expression.is_null())
		return gmt::nullref;

	auto it = m_expressionTypes.find(expression);
	return it != m_expressionTypes.end() ? it->second : gmt::nullref;
}

void SemanticModel::set_expression_type(gmt::Ref<const ExpressionSyntax> expression, gmt::Ref<StructSymbol> type)
{
	if (!expression.is_null())
		m_expressionTypes[expression] = type;
}

bool SemanticModel::are_types_equal(gmt::Ref<const StructSymbol> left, gmt::Ref<const StructSymbol> right)
{
	if (left.is_null() || right.is_null())
		return false;

	switch (left->get_kind())
	{
		default:
			return left == right;

		case SyntaxKind::GenericType:
		{
			if (right->get_kind() != SyntaxKind::GenericType)
				return false;

			gmt::Ref<const GenericTypeSymbol> left_generic = left.as<GenericTypeSymbol>();
			gmt::Ref<const GenericTypeSymbol> right_generic = right.as<GenericTypeSymbol>();

			gmt::Ref<const StructSymbol> left_underlaying = left_generic->get_underlaying_type();
			gmt::Ref<const StructSymbol> right_underlaying = right_generic->get_underlaying_type();

			if (!are_types_equal(left_underlaying, right_underlaying))
				return false;

			std::size_t size = left_underlaying->type_parameters.size();
			if (right_underlaying->type_parameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				gmt::Ref<StructSymbol> left_argument = left_generic->substitute_type_parameter(left_underlaying->type_parameters[i]);
				gmt::Ref<StructSymbol> right_argument = right_generic->substitute_type_parameter(right_underlaying->type_parameters[i]);

				if (!are_types_equal(left_argument, right_argument))
					return false;
			}

			return true;
		}

		case SyntaxKind::DelegateType:
		{
			if (right->get_kind() != SyntaxKind::DelegateType)
				return false;

			gmt::Ref<const DelegateTypeSymbol> left_delegate = left.as<DelegateTypeSymbol>();
			gmt::Ref<const DelegateTypeSymbol> right_delegate = right.as<DelegateTypeSymbol>();

			if (left_delegate->get_return_type().is_null() || right_delegate->get_return_type().is_null())
				return false;

			if (!are_types_equal(left_delegate->get_return_type(), right_delegate->get_return_type()))
				return false;

			std::size_t size = left_delegate->get_parameters().size();
			if (right_delegate->get_parameters().size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				if (!are_types_equal(left_delegate->get_parameters()[i]->get_type(), right_delegate->get_parameters()[i]->get_type()))
					return false;
			}

			return true;
		}

		case SyntaxKind::ArrayType:
		{
			if (right->get_kind() != SyntaxKind::ArrayType)
				return false;

			gmt::Ref<const ArrayTypeSymbol> left_array = left.as<ArrayTypeSymbol>();
			gmt::Ref<const ArrayTypeSymbol> right_array = right.as<ArrayTypeSymbol>();

			if (left_array->get_underlaying_type().is_null() || right_array->get_underlaying_type().is_null())
				return false;

			return are_types_equal(left_array->get_underlaying_type(), right_array->get_underlaying_type());
		}
	}
}

static void collect_all_interfaces(gmt::Ref<const StructSymbol> type, std::vector<gmt::Ref<const StructSymbol>>& out, std::unordered_set<gmt::Ref<const StructSymbol>>& visited)
{
	if (type.is_null())
		return;

	for (gmt::Ref<StructSymbol> interface : type->interfaces)
	{
		if (interface.is_null())
			continue;

		gmt::Ref<const StructSymbol> const_interface = interface;
		if (visited.insert(const_interface).second)
		{
			out.push_back(const_interface);
			collect_all_interfaces(const_interface, out, visited);
		}
	}
}

static std::vector<gmt::Ref<const StructSymbol>> get_all_interfaces(gmt::Ref<const StructSymbol> type)
{
	std::vector<gmt::Ref<const StructSymbol>> result;
	std::unordered_set<gmt::Ref<const StructSymbol>> visited;
	collect_all_interfaces(type, result, visited);
	return result;
}

static bool is_constructed_interface_implemented(gmt::Ref<GenericTypeSymbol> target_interface, gmt::Ref<const StructSymbol> source_type)
{
	gmt::Ref<StructSymbol> underlaying = target_interface->get_underlaying_type();
	if (underlaying->get_kind() != SyntaxKind::InterfaceDeclaration)
		return false;

	auto matches_interface = [&](gmt::Ref<GenericTypeSymbol> candidate_interface) -> bool
	{
		if (candidate_interface->get_underlaying_type() != underlaying)
			return false;

		for (gmt::Ref<TypeParameterSymbol> parameter : underlaying->type_parameters)
		{
			gmt::Ref<StructSymbol> target_argument = target_interface->substitute_type_parameter(parameter);
			gmt::Ref<StructSymbol> source_argument = candidate_interface->substitute_type_parameter(parameter);

			if (!source_argument.is_null() && source_argument->get_kind() == SyntaxKind::TypeParameter && !source_type.is_null() && source_type->get_kind() == SyntaxKind::GenericType)
				source_argument = source_type.as<GenericTypeSymbol>()->substitute_type_parameter(source_argument.as<TypeParameterSymbol>());

			if (!SemanticModel::are_types_equal(target_argument, source_argument))
				return false;
		}

		return true;
	};

	auto check_type_interfaces = [&](gmt::Ref<const StructSymbol> type) -> bool
	{
		for (gmt::Ref<const StructSymbol> interface : get_all_interfaces(type))
		{
			if (interface->get_kind() != SyntaxKind::GenericType)
				continue;

			if (matches_interface(interface.as<GenericTypeSymbol>()))
				return true;
		}

		return false;
	};

	if (check_type_interfaces(source_type))
		return true;

	if (source_type->get_kind() == SyntaxKind::GenericType)
	{
		if (check_type_interfaces(source_type.as<GenericTypeSymbol>()->get_underlaying_type()))
			return true;
	}

	return false;
}

bool SemanticModel::is_assignable_to(gmt::Ref<const StructSymbol> target, gmt::Ref<const StructSymbol> source) const
{
	if (target.is_null() || source.is_null())
		return false;

	if (are_types_equal(target, source))
		return true;

	if (target->get_kind() == SyntaxKind::InterfaceDeclaration)
	{
		for (gmt::Ref<const StructSymbol> interface : get_all_interfaces(source))
		{
			if (are_types_equal(target, interface))
				return true;
		}

		if (source->get_kind() == SyntaxKind::ArrayType)
		{
			for (gmt::Ref<const StructSymbol> interface : get_all_interfaces(m_table.get_array_primitive()))
			{
				if (are_types_equal(target, interface))
					return true;
			}
		}
	}

	if (target->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<GenericTypeSymbol> target_generic = target.as<GenericTypeSymbol>();
		if (is_constructed_interface_implemented(target_generic, source))
			return true;

		if (target_generic->get_underlaying_type()->get_kind() == SyntaxKind::DelegateType && source->get_kind() == SyntaxKind::DelegateType)
		{
			gmt::Ref<const DelegateTypeSymbol> target_delegate = target_generic->get_underlaying_type().as<DelegateTypeSymbol>();
			gmt::Ref<const DelegateTypeSymbol> source_delegate = source.as<DelegateTypeSymbol>();

			if (target_delegate->get_return_type().is_null() || source_delegate->get_return_type().is_null())
				return false;

			gmt::Ref<StructSymbol> target_return = target_delegate->get_return_type();
			if (target_return->get_kind() == SyntaxKind::TypeParameter)
			{
				gmt::Ref<StructSymbol> substituted = target_generic->substitute_type_parameter(target_return.as<TypeParameterSymbol>());
				if (!substituted.is_null())
					target_return = substituted;
			}

			if (!are_types_equal(target_return, source_delegate->get_return_type()))
				return false;

			if (target_delegate->get_parameters().size() != source_delegate->get_parameters().size())
				return false;

			for (std::size_t i = 0; i < target_delegate->get_parameters().size(); ++i)
			{
				gmt::Ref<StructSymbol> target_parameter = target_delegate->get_parameters()[i]->get_type();
				if (target_parameter->get_kind() == SyntaxKind::TypeParameter)
				{
					gmt::Ref<StructSymbol> substituted = target_generic->substitute_type_parameter(target_parameter.as<TypeParameterSymbol>());
					if (!substituted.is_null())
						target_parameter = substituted;
				}

				if (!are_types_equal(target_parameter, source_delegate->get_parameters()[i]->get_type()))
					return false;
			}

			return true;
		}
	}

	if (target->get_kind() == SyntaxKind::DelegateType && source->get_kind() == SyntaxKind::DelegateType)
	{
		gmt::Ref<const DelegateTypeSymbol> target_delegate = target.as<DelegateTypeSymbol>();
		gmt::Ref<const DelegateTypeSymbol> source_delegate = source.as<DelegateTypeSymbol>();

		if (!are_types_equal(target_delegate->get_return_type(), source_delegate->get_return_type()))
			return false;

		if (target_delegate->get_parameters().size() != source_delegate->get_parameters().size())
			return false;

		for (std::size_t i = 0; i < target_delegate->get_parameters().size(); ++i)
		{
			if (!are_types_equal(target_delegate->get_parameters()[i]->get_type(), source_delegate->get_parameters()[i]->get_type()))
				return false;
		}

		return true;
	}

	return false;
}

std::wstring SemanticModel::get_type_display_name(gmt::Ref<const StructSymbol> type) const
{
	if (type.is_null())
		return L"<error>";

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<const GenericTypeSymbol> generic = type.as<GenericTypeSymbol>();
		gmt::Ref<const StructSymbol> underlaying = generic->get_underlaying_type();

		std::wstring result = underlaying.is_null() ? L"<error>" : std::wstring(underlaying->get_name());
		result += L'<';

		if (!underlaying.is_null())
		{
			bool first = true;
			for (gmt::Ref<TypeParameterSymbol> parameter : underlaying->type_parameters)
			{
				if (!first)
					result += L", ";

				gmt::Ref<StructSymbol> argument = generic->substitute_type_parameter(parameter);
				result += get_type_display_name(argument.is_null() ? gmt::Ref<const StructSymbol>(parameter) : argument);
				first = false;
			}
		}

		result += L'>';
		return result;
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<const ArrayTypeSymbol> array = type.as<ArrayTypeSymbol>();
		return gmt::concat(get_type_display_name(array->get_underlaying_type()), L"[]");
	}

	return std::wstring(type->get_name());
}

bool SemanticModel::is_primitive_type(gmt::Ref<const StructSymbol> type) const
{
	return type == m_table.get_boolean_primitive()
		|| type == m_table.get_integer_primitive()
		|| type == m_table.get_double_primitive()
		|| type == m_table.get_char_primitive()
		|| type == m_table.get_string_primitive()
		|| type == m_table.get_array_primitive()
		|| type == m_table.get_nint_primitive();
}

gmt::Ref<StructSymbol> SemanticModel::substitute_type(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> within)
{
	if (type.is_null())
		return gmt::nullref;

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
			gmt::Ref<ArrayTypeSymbol> array_type = type.as<ArrayTypeSymbol>();
			gmt::Ref<StructSymbol> underlaying_type = array_type->get_underlaying_type();

			if (underlaying_type.is_null())
				throw std::runtime_error("cannot resolve underlaying type of array");

			if (underlaying_type->get_kind() == SyntaxKind::TypeParameter)
			{
				if (within.is_null())
					return type;

				underlaying_type = within->substitute_type_parameter(underlaying_type.as<TypeParameterSymbol>());
				if (underlaying_type.is_null())
					throw std::runtime_error("cannot resolve underlaying type of array");

				return m_table.find_or_create_array_type(underlaying_type, array_type->get_length());
			}

			return array_type;
		}

		case SyntaxKind::TypeParameter:
		{
			if (within.is_null())
				return type;

			gmt::Ref<StructSymbol> resolved_type = within->substitute_type_parameter(type.as<TypeParameterSymbol>());
			return !resolved_type.is_null() ? resolved_type : type;
		}
	}
}

gmt::Ref<StructSymbol> SemanticModel::get_field_type(gmt::Ref<FieldSymbol> field, gmt::Ref<GenericTypeSymbol> within)
{
	if (field.is_null() || field->get_return_type().is_null())
		return gmt::nullref;

	return substitute_type(field->get_return_type(), within);
}

gmt::Ref<StructSymbol> SemanticModel::get_method_return_type(gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> within)
{
	if (method.is_null() || method->get_return_type().is_null())
		return gmt::nullref;

	return substitute_type(method->get_return_type(), within);
}

gmt::Ref<StructSymbol> SemanticModel::get_property_type(gmt::Ref<PropertySymbol> property, gmt::Ref<GenericTypeSymbol> within)
{
	if (property.is_null() || property->get_return_type().is_null())
		return gmt::nullref;

	return substitute_type(property->get_return_type(), within);
}

gmt::Ref<StructSymbol> SemanticModel::get_constructor_return_type(gmt::Ref<ConstructorSymbol> constructor, gmt::Ref<GenericTypeSymbol> within)
{
	if (constructor.is_null() || constructor->get_return_type().is_null())
		return gmt::nullref;

	return substitute_type(constructor->get_return_type(), within);
}

static gmt::Ref<StructSymbol> find_type_in_namespace(gmt::Ref<NamespaceSymbol> scope, std::wstring_view name)
{
	if (scope.is_null())
		return gmt::nullref;

	for (gmt::Ref<SyntaxSymbol> member : scope->members)
	{
		if (!member.is_null() && member->is_type() && member->get_name() == name)
			return member.as<StructSymbol>();
	}

	for (const auto& child : scope->get_children())
	{
		gmt::Ref<StructSymbol> result = find_type_in_namespace(child.second, name);
		if (!result.is_null())
			return result;
	}

	return gmt::nullref;
}

gmt::Ref<StructSymbol> SemanticModel::find_type_by_name(std::wstring_view full_name)
{
	gmt::Ref<StructSymbol> primitives[] = {
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

	std::size_t dot = full_name.find(L'.');
	if (dot == std::wstring_view::npos)
	{
		for (gmt::Ref<StructSymbol> primitive : primitives)
		{
			if (!primitive.is_null() && primitive->get_name() == full_name)
				return primitive;
		}

		return find_type_in_namespace(m_table.get_root_namespace(), full_name);
	}

	gmt::Ref<NamespaceSymbol> node = m_table.get_root_namespace();
	std::size_t start = 0;
	std::size_t end = full_name.find(L'.');

	while (!node.is_null() && end != std::wstring_view::npos)
	{
		node = node->lookup(full_name.substr(start, end - start));
		start = end + 1;
		end = full_name.find(L'.', start);
	}

	if (!node.is_null())
	{
		std::wstring_view type_name = full_name.substr(start);
		for (gmt::Ref<SyntaxSymbol> member : node->members)
		{
			if (member.is_null() || !member->is_type() || member->get_name() != type_name)
				continue;

			return member.as<StructSymbol>();
		}
	}

	return gmt::nullref;
}

gmt::Ref<FieldSymbol> SemanticModel::find_field_by_name(gmt::Ref<StructSymbol> type, std::wstring_view name) const
{
	if (type.is_null())
		return gmt::nullref;

	for (gmt::Ref<FieldSymbol> field : type->fields)
	{
		if (field->get_name() == name)
			return field;
	}

	return gmt::nullref;
}

gmt::Ref<StructSymbol> SemanticModel::resolve_runtime_type_argument(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolve_parameter)
{
	if (type.is_null())
		return type;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> resolved = resolve_parameter(type.as<TypeParameterSymbol>());
		return !resolved.is_null() ? resolved : type;
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<ArrayTypeSymbol> array_type = type.as<ArrayTypeSymbol>();
		gmt::Ref<StructSymbol> resolved_underlaying = resolve_runtime_type_argument(array_type->get_underlaying_type(), resolve_parameter);

		if (resolved_underlaying.is_null() || resolved_underlaying == array_type->get_underlaying_type())
			return type;

		return m_table.find_or_create_array_type(resolved_underlaying, array_type->get_length());
	}

	return type;
}

bool SemanticModel::try_resolve_generic_arguments(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolve_parameter, gmt::Ref<StructSymbol>& base_type, std::vector<gmt::Ref<StructSymbol>>& generic_args)
{
	if (type.is_null())
		return false;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> resolved = resolve_parameter(type.as<TypeParameterSymbol>());
		if (!resolved.is_null() && resolved != type)
			return try_resolve_generic_arguments(resolved, resolve_parameter, base_type, generic_args);

		return false;
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<GenericTypeSymbol> generic = type.as<GenericTypeSymbol>();
		base_type = generic->get_underlaying_type();

		generic_args.clear();
		generic_args.reserve(base_type->type_parameters.size());
		for (gmt::Ref<TypeParameterSymbol> parameter : base_type->type_parameters)
		{
			gmt::Ref<StructSymbol> argument = generic->substitute_type_parameter(parameter);
			gmt::Ref<StructSymbol> resolved_argument = resolve_runtime_type_argument(argument, resolve_parameter);
			generic_args.push_back(!resolved_argument.is_null() ? resolved_argument : argument);
		}

		return true;
	}

	return false;
}
