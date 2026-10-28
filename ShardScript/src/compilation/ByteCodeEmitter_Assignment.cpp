#include <shard/compilation/ByteCodeEmitter.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>

#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <shard/pragmatic/PlacementVerdict.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

using namespace shard;

void ByteCodeEmitter::emit_assignment(const BinaryExpressionSyntax* node)
{
	const ExpressionSyntax* target = node->get_left();
	std::optional<const BinaryExpressionSyntax*> nodeRef = ref_from(node);

	if (target->get_kind() == SyntaxKind::IndexatorExpression)
	{
		const IndexatorExpressionSyntax* indexator = static_cast<const IndexatorExpressionSyntax*>(target);
		std::optional<const IndexatorExpressionSyntax*> indexatorRef = ref_from(indexator);

		IndexatorSymbol* symbol = indexatorRef.has_value() ? lookup_symbol<IndexatorSymbol>(*indexatorRef) : nullptr;
		if (!(symbol == nullptr))
		{
			MethodSymbol* setter = symbol->get_setter();
			if ((setter == nullptr))
			{
				unsupported(*nodeRef, L"indexator without a setter");
				return;
			}

			StructSymbol* receiverType = (indexator->get_previous() == nullptr) ? nullptr : get_model().get_expression_type(indexator->get_previous());
			setter = resolve_call_target(*nodeRef, setter, receiverType);
			if ((setter == nullptr))
				return;

			node->get_right()->accept(*this);
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));

			emit_arguments_reversed(indexator->get_arguments());
			if (!(indexator->get_previous() == nullptr))
				indexator->get_previous()->accept(*this);
			else
			{
				m_encoder.emit_load_local(0);
				eval_push(sizeof(void*));
			}

			m_encoder.emit_call_method_symbol(setter);
			eval_pop(symbol->get_parameters().size() + 2);
			return;
		}

		indexator->get_previous()->accept(*this);

		std::span<ArgumentSyntax* const> arguments = indexator->get_arguments()->get_arguments();
		if (!arguments.empty())
			arguments.front()->get_expression()->accept(*this);

		node->get_right()->accept(*this);

		StructSymbol* elementType = nullptr;
		if (!(indexator->get_previous() == nullptr))
			elementType = get_model().get_expression_type(indexator->get_previous());

		if (!(elementType == nullptr) && elementType->get_kind() == SyntaxKind::ArrayType)
			elementType = static_cast<ArrayTypeSymbol*>(elementType)->get_underlaying_type();

		std::uint16_t temp = allocate_temporary(elementType, PlacementVerdict::Heap);
		m_encoder.emit_duplicate();
		eval_push(sizeof(void*));
		m_encoder.emit_store_local(temp);
		eval_pop();

		m_encoder.emit_store_array_element();
		eval_pop(3);

		m_encoder.emit_load_local(temp);
		eval_push(sizeof(void*));
		return;
	}

	if (!is_identifier_target(target))
	{
		unsupported(*nodeRef, L"this assignment target");
		return;
	}

	const ExpressionSyntax* previous = nullptr;
	if (target->get_kind() == SyntaxKind::MemberAccessExpression)
		previous = static_cast<const MemberAccessExpressionSyntax*>(target)->get_previous();

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(target);
	if ((bound == nullptr))
		return;

	SyntaxKind kind = bound->get_kind();
	if (kind == SyntaxKind::VariableStatement || kind == SyntaxKind::Parameter)
	{
		std::optional<std::uint16_t> slot = slot_of(bound);
		if (slot.has_value())
		{
			node->get_right()->accept(*this);
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_local(*slot);
			eval_pop();
			return;
		}

		FieldSymbol* captureField = capture_field_of(bound);
		if (!(captureField == nullptr))
		{
			// the captured variable is a field of the box receiver (arg 0)
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));

			node->get_right()->accept(*this);

			std::uint16_t temp = allocate_temporary(captureField->get_return_type(), PlacementVerdict::Heap);
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_local(temp);
			eval_pop();

			m_encoder.emit_store_field(field_slot_index(captureField));
			eval_pop(2);

			m_encoder.emit_load_local(temp);
			eval_push(sizeof(void*));
			return;
		}

		unsupported(*nodeRef, L"lambda captures");
		return;
	}

	if (kind == SyntaxKind::FieldDeclaration)
	{
		FieldSymbol* field = static_cast<FieldSymbol*>(bound);
		if (field->get_is_enum_value())
			return;

		if (field->get_linking() == LINK_STATIC)
		{
			node->get_right()->accept(*this);
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_static_field(field);
			eval_pop();
			return;
		}

		if (!(previous == nullptr) && is_identifier_target(previous))
		{
			SyntaxSymbol* previousBound = get_symbol_table().lookup_symbol(previous);
			if (!(previousBound == nullptr) && local_verdict(previousBound) == PlacementVerdict::Scalar)
			{
				std::optional<std::uint16_t> scalarSlot = scalar_field_slot(previousBound, field);
				if (scalarSlot.has_value())
				{
					node->get_right()->accept(*this);
					m_encoder.emit_duplicate();
					eval_push(sizeof(void*));
					m_encoder.emit_store_local(*scalarSlot);
					eval_pop();
					return;
				}
			}
		}

		if (!(previous == nullptr))
			previous->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		node->get_right()->accept(*this);

		std::uint16_t temp = allocate_temporary(field->get_return_type(), PlacementVerdict::Heap);
		m_encoder.emit_duplicate();
		eval_push(sizeof(void*));
		m_encoder.emit_store_local(temp);
		eval_pop();

		m_encoder.emit_store_field(field_slot_index(field));
		eval_pop(2);

		m_encoder.emit_load_local(temp);
		eval_push(sizeof(void*));
		return;
	}

	if (kind == SyntaxKind::PropertyDeclaration)
	{
		PropertySymbol* property = static_cast<PropertySymbol*>(bound);
		MethodSymbol* setter = property->get_setter();
		if ((setter == nullptr))
		{
			unsupported(*nodeRef, L"property without a setter");
			return;
		}

		StructSymbol* receiverType = (previous == nullptr) ? nullptr : get_model().get_expression_type(previous);
		setter = resolve_call_target(*nodeRef, setter, receiverType);
		if ((setter == nullptr))
			return;

		node->get_right()->accept(*this);

		std::uint16_t temp = allocate_temporary(property->get_return_type(), PlacementVerdict::Heap);
		m_encoder.emit_duplicate();
		eval_push(sizeof(void*));
		m_encoder.emit_store_local(temp);
		eval_pop();

		if (!(previous == nullptr))
			previous->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		m_encoder.emit_call_method_symbol(setter);
		eval_pop(2);

		m_encoder.emit_load_local(temp);
		eval_push(sizeof(void*));
		return;
	}

	unsupported(*nodeRef, L"this assignment target");
}

void ByteCodeEmitter::emit_compound_assignment(const BinaryExpressionSyntax* node, TokenType arithmetic)
{
	const ExpressionSyntax* target = node->get_left();
	std::optional<const BinaryExpressionSyntax*> nodeRef = ref_from(node);

	if (!is_identifier_target(target))
	{
		unsupported(*nodeRef, L"compound assignment on this target");
		return;
	}

	const ExpressionSyntax* previous = nullptr;
	if (target->get_kind() == SyntaxKind::MemberAccessExpression)
		previous = static_cast<const MemberAccessExpressionSyntax*>(target)->get_previous();

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(target);
	if ((bound == nullptr))
		return;

	SyntaxKind kind = bound->get_kind();
	if (kind == SyntaxKind::VariableStatement || kind == SyntaxKind::Parameter)
	{
		std::optional<std::uint16_t> slot = slot_of(bound);
		if (slot.has_value())
		{
			m_encoder.emit_load_local(*slot);
			eval_push(sizeof(void*));

			node->get_right()->accept(*this);
			emit_arithmetic_op(arithmetic);
			eval_pop();

			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_local(*slot);
			eval_pop();
			return;
		}

		FieldSymbol* captureField = capture_field_of(bound);
		if (!(captureField == nullptr))
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));

			m_encoder.emit_load_field(field_slot_index(captureField));
			eval_push(payload_of(captureField->get_return_type()));
			eval_pop();

			node->get_right()->accept(*this);
			emit_arithmetic_op(arithmetic);
			eval_pop();

			std::uint16_t temp = allocate_temporary(captureField->get_return_type(), PlacementVerdict::Heap);
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_local(temp);
			eval_pop();

			m_encoder.emit_store_field(field_slot_index(captureField));
			eval_pop(2);

			m_encoder.emit_load_local(temp);
			eval_push(sizeof(void*));
			return;
		}

		unsupported(*nodeRef, L"lambda captures");
		return;
	}

	if (kind == SyntaxKind::FieldDeclaration)
	{
		FieldSymbol* field = static_cast<FieldSymbol*>(bound);
		if (field->get_linking() == LINK_STATIC)
		{
			m_encoder.emit_load_static_field(field);
			eval_push(sizeof(void*));

			node->get_right()->accept(*this);
			emit_arithmetic_op(arithmetic);
			eval_pop();

			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_store_static_field(field);
			eval_pop();
			return;
		}

		if (!(previous == nullptr))
			previous->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		m_encoder.emit_duplicate();
		eval_push(sizeof(void*));

		m_encoder.emit_load_field(field_slot_index(field));
		eval_push(payload_of(field->get_return_type()));
		eval_pop();

		node->get_right()->accept(*this);
		emit_arithmetic_op(arithmetic);
		eval_pop();

		std::uint16_t temp = allocate_temporary(field->get_return_type(), PlacementVerdict::Heap);
		m_encoder.emit_duplicate();
		eval_push(sizeof(void*));
		m_encoder.emit_store_local(temp);
		eval_pop();

		m_encoder.emit_store_field(field_slot_index(field));
		eval_pop(2);

		m_encoder.emit_load_local(temp);
		eval_push(sizeof(void*));
		return;
	}

	unsupported(*nodeRef, L"compound assignment on this target");
}