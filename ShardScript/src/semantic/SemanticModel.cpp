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
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
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

			gmt::Ref<const GenericTypeSymbol> leftGeneric = left.as<GenericTypeSymbol>();
			gmt::Ref<const GenericTypeSymbol> rightGeneric = right.as<GenericTypeSymbol>();

			gmt::Ref<const StructSymbol> leftUnderlaying = leftGeneric->get_underlaying_type();
			gmt::Ref<const StructSymbol> rightUnderlaying = rightGeneric->get_underlaying_type();

			if (!are_types_equal(leftUnderlaying, rightUnderlaying))
				return false;

			std::size_t size = leftUnderlaying->type_parameters.size();
			if (rightUnderlaying->type_parameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				gmt::Ref<StructSymbol> leftArgument = leftGeneric->substitute_type_parameter(leftUnderlaying->type_parameters[i]);
				gmt::Ref<StructSymbol> rightArgument = rightGeneric->substitute_type_parameter(rightUnderlaying->type_parameters[i]);

				if (!are_types_equal(leftArgument, rightArgument))
					return false;
			}

			return true;
		}

		case SyntaxKind::DelegateType:
		{
			if (right->get_kind() != SyntaxKind::DelegateType)
				return false;

			gmt::Ref<const DelegateTypeSymbol> leftDelegate = left.as<DelegateTypeSymbol>();
			gmt::Ref<const DelegateTypeSymbol> rightDelegate = right.as<DelegateTypeSymbol>();

			if (leftDelegate->get_return_type().is_null() || rightDelegate->get_return_type().is_null())
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

			gmt::Ref<const ArrayTypeSymbol> leftArray = left.as<ArrayTypeSymbol>();
			gmt::Ref<const ArrayTypeSymbol> rightArray = right.as<ArrayTypeSymbol>();

			if (leftArray->get_underlaying_type().is_null() || rightArray->get_underlaying_type().is_null())
				return false;

			return are_types_equal(leftArray->get_underlaying_type(), rightArray->get_underlaying_type());
		}

		case SyntaxKind::NullableType:
		{
			if (right->get_kind() != SyntaxKind::NullableType)
				return false;

			return are_types_equal(left.as<NullableTypeSymbol>()->get_underlaying_type(), right.as<NullableTypeSymbol>()->get_underlaying_type());
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

		gmt::Ref<const StructSymbol> constInterface = interface;
		if (visited.insert(constInterface).second)
		{
			out.push_back(constInterface);
			collect_all_interfaces(constInterface, out, visited);
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

static bool is_constructed_interface_implemented(gmt::Ref<GenericTypeSymbol> targetInterface, gmt::Ref<const StructSymbol> sourceType)
{
	gmt::Ref<StructSymbol> underlaying = targetInterface->get_underlaying_type();
	if (underlaying->get_kind() != SyntaxKind::InterfaceDeclaration)
		return false;

	auto matchesInterface = [&](gmt::Ref<GenericTypeSymbol> candidateInterface) -> bool
	{
		if (candidateInterface->get_underlaying_type() != underlaying)
			return false;

		for (gmt::Ref<TypeParameterSymbol> parameter : underlaying->type_parameters)
		{
			gmt::Ref<StructSymbol> targetArgument = targetInterface->substitute_type_parameter(parameter);
			gmt::Ref<StructSymbol> sourceArgument = candidateInterface->substitute_type_parameter(parameter);

			if (!sourceArgument.is_null() && sourceArgument->get_kind() == SyntaxKind::TypeParameter && !sourceType.is_null() && sourceType->get_kind() == SyntaxKind::GenericType)
				sourceArgument = sourceType.as<GenericTypeSymbol>()->substitute_type_parameter(sourceArgument.as<TypeParameterSymbol>());

			if (!SemanticModel::are_types_equal(targetArgument, sourceArgument))
				return false;
		}

		return true;
	};

	auto checkTypeInterfaces = [&](gmt::Ref<const StructSymbol> type) -> bool
	{
		for (gmt::Ref<const StructSymbol> interface : get_all_interfaces(type))
		{
			if (interface->get_kind() != SyntaxKind::GenericType)
				continue;

			if (matchesInterface(interface.as<GenericTypeSymbol>()))
				return true;
		}

		return false;
	};

	if (checkTypeInterfaces(sourceType))
		return true;

	if (sourceType->get_kind() == SyntaxKind::GenericType)
	{
		if (checkTypeInterfaces(sourceType.as<GenericTypeSymbol>()->get_underlaying_type()))
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

	// TODO: pragmatic analysis — flow-based null-state checks
	if (source->get_kind() == SyntaxKind::NullableType && target->get_kind() != SyntaxKind::NullableType)
		return is_assignable_to(target, source.as<NullableTypeSymbol>()->get_underlaying_type());

	if (source->get_kind() != SyntaxKind::NullableType && target->get_kind() == SyntaxKind::NullableType)
		return is_assignable_to(target.as<NullableTypeSymbol>()->get_underlaying_type(), source);

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
		gmt::Ref<GenericTypeSymbol> targetGeneric = target.as<GenericTypeSymbol>();
		if (is_constructed_interface_implemented(targetGeneric, source))
			return true;

		if (targetGeneric->get_underlaying_type()->get_kind() == SyntaxKind::DelegateType && source->get_kind() == SyntaxKind::DelegateType)
		{
			gmt::Ref<const DelegateTypeSymbol> targetDelegate = targetGeneric->get_underlaying_type().as<DelegateTypeSymbol>();
			gmt::Ref<const DelegateTypeSymbol> sourceDelegate = source.as<DelegateTypeSymbol>();

			if (targetDelegate->get_return_type().is_null() || sourceDelegate->get_return_type().is_null())
				return false;

			gmt::Ref<StructSymbol> targetReturn = targetDelegate->get_return_type();
			if (targetReturn->get_kind() == SyntaxKind::TypeParameter)
			{
				gmt::Ref<StructSymbol> substituted = targetGeneric->substitute_type_parameter(targetReturn.as<TypeParameterSymbol>());
				if (!substituted.is_null())
					targetReturn = substituted;
			}

			if (!are_types_equal(targetReturn, sourceDelegate->get_return_type()))
				return false;

			if (targetDelegate->get_parameters().size() != sourceDelegate->get_parameters().size())
				return false;

			for (std::size_t i = 0; i < targetDelegate->get_parameters().size(); ++i)
			{
				gmt::Ref<StructSymbol> targetParameter = targetDelegate->get_parameters()[i]->get_type();
				if (targetParameter->get_kind() == SyntaxKind::TypeParameter)
				{
					gmt::Ref<StructSymbol> substituted = targetGeneric->substitute_type_parameter(targetParameter.as<TypeParameterSymbol>());
					if (!substituted.is_null())
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
		gmt::Ref<const DelegateTypeSymbol> targetDelegate = target.as<DelegateTypeSymbol>();
		gmt::Ref<const DelegateTypeSymbol> sourceDelegate = source.as<DelegateTypeSymbol>();

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

	if (type->get_kind() == SyntaxKind::NullableType)
	{
		gmt::Ref<const NullableTypeSymbol> nullable = type.as<NullableTypeSymbol>();
		return gmt::concat(get_type_display_name(nullable->get_underlaying_type()), L"?");
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
			gmt::Ref<ArrayTypeSymbol> arrayType = type.as<ArrayTypeSymbol>();
			gmt::Ref<StructSymbol> underlayingType = arrayType->get_underlaying_type();

			if (underlayingType.is_null())
				throw std::runtime_error("cannot resolve underlaying type of array");

			if (underlayingType->get_kind() == SyntaxKind::TypeParameter)
			{
				if (within.is_null())
					return type;

				underlayingType = within->substitute_type_parameter(underlayingType.as<TypeParameterSymbol>());
				if (underlayingType.is_null())
					throw std::runtime_error("cannot resolve underlaying type of array");

				return m_table.find_or_create_array_type(underlayingType, arrayType->get_length());
			}

			return arrayType;
		}

		case SyntaxKind::TypeParameter:
		{
			if (within.is_null())
				return type;

			gmt::Ref<StructSymbol> resolvedType = within->substitute_type_parameter(type.as<TypeParameterSymbol>());
			return !resolvedType.is_null() ? resolvedType : type;
		}

		case SyntaxKind::NullableType:
		{
			gmt::Ref<NullableTypeSymbol> nullable = type.as<NullableTypeSymbol>();
			gmt::Ref<StructSymbol> underlaying = substitute_type(nullable->get_underlaying_type(), within);

			if (underlaying.is_null() || underlaying == nullable->get_underlaying_type())
				return type;

			return m_table.find_or_create_nullable_type(underlaying);
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

gmt::Ref<StructSymbol> SemanticModel::find_type_by_name(std::wstring_view fullName)
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

	std::size_t dot = fullName.find(L'.');
	if (dot == std::wstring_view::npos)
	{
		for (gmt::Ref<StructSymbol> primitive : primitives)
		{
			if (!primitive.is_null() && primitive->get_name() == fullName)
				return primitive;
		}

		return find_type_in_namespace(m_table.get_root_namespace(), fullName);
	}

	gmt::Ref<NamespaceSymbol> node = m_table.get_root_namespace();
	std::size_t start = 0;
	std::size_t end = fullName.find(L'.');

	while (!node.is_null() && end != std::wstring_view::npos)
	{
		node = node->lookup(fullName.substr(start, end - start));
		start = end + 1;
		end = fullName.find(L'.', start);
	}

	if (!node.is_null())
	{
		std::wstring_view typeName = fullName.substr(start);
		for (gmt::Ref<SyntaxSymbol> member : node->members)
		{
			if (member.is_null() || !member->is_type() || member->get_name() != typeName)
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

gmt::Ref<StructSymbol> SemanticModel::resolve_runtime_type_argument(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter)
{
	if (type.is_null())
		return type;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> resolved = resolveParameter(type.as<TypeParameterSymbol>());
		return !resolved.is_null() ? resolved : type;
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<ArrayTypeSymbol> arrayType = type.as<ArrayTypeSymbol>();
		gmt::Ref<StructSymbol> resolvedUnderlaying = resolve_runtime_type_argument(arrayType->get_underlaying_type(), resolveParameter);

		if (resolvedUnderlaying.is_null() || resolvedUnderlaying == arrayType->get_underlaying_type())
			return type;

		return m_table.find_or_create_array_type(resolvedUnderlaying, arrayType->get_length());
	}

	return type;
}

bool SemanticModel::try_resolve_generic_arguments(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter, gmt::Ref<StructSymbol>& baseType, std::vector<gmt::Ref<StructSymbol>>& genericArgs)
{
	if (type.is_null())
		return false;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> resolved = resolveParameter(type.as<TypeParameterSymbol>());
		if (!resolved.is_null() && resolved != type)
			return try_resolve_generic_arguments(resolved, resolveParameter, baseType, genericArgs);

		return false;
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<GenericTypeSymbol> generic = type.as<GenericTypeSymbol>();
		baseType = generic->get_underlaying_type();

		genericArgs.clear();
		genericArgs.reserve(baseType->type_parameters.size());
		for (gmt::Ref<TypeParameterSymbol> parameter : baseType->type_parameters)
		{
			gmt::Ref<StructSymbol> argument = generic->substitute_type_parameter(parameter);
			gmt::Ref<StructSymbol> resolvedArgument = resolve_runtime_type_argument(argument, resolveParameter);
			genericArgs.push_back(!resolvedArgument.is_null() ? resolvedArgument : argument);
		}

		return true;
	}

	return false;
}
