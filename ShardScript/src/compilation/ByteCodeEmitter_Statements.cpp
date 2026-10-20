#include <shard/compilation/ByteCodeEmitter.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>

#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

using namespace shard;

namespace
{
	// accessors belong to a property or indexator, so the declaring type sits
	// one level above the accessor's direct parent
	bool is_interface_accessor(gmt::Ref<MethodSymbol> accessor)
	{
		gmt::Ref<const SyntaxSymbol> owner = accessor->get_parent();
		if (!owner.is_null() && (owner->get_kind() == SyntaxKind::PropertyDeclaration || owner->get_kind() == SyntaxKind::IndexatorDeclaration))
			owner = owner->get_parent();

		return !owner.is_null() && owner->get_kind() == SyntaxKind::InterfaceDeclaration;
	}
}

void ByteCodeEmitter::emit_defer_drain(std::size_t count)
{
	if (count == 0)
		return;

	m_encoder.emit_defer_drain(count);
}

void ByteCodeEmitter::emit_scope_defers()
{
	if (m_deferScopes.empty())
		return;

	emit_defer_drain(m_deferScopes.back());
	m_deferScopes.pop_back();
}

void ByteCodeEmitter::emit_loop_defers()
{
	if (m_loops.empty())
		return;

	std::size_t depth = m_loops.back().deferScopeDepth;
	std::size_t count = 0;
	for (std::size_t i = depth; i < m_deferScopes.size(); ++i)
		count += m_deferScopes[i];

	emit_defer_drain(count);
}

void ByteCodeEmitter::emit_all_defers()
{
	std::size_t count = 0;
	for (std::size_t scope : m_deferScopes)
		count += scope;

	emit_defer_drain(count);
}

void ByteCodeEmitter::register_defer()
{
	if (m_deferScopes.empty())
		m_deferScopes.push_back(0);

	++m_deferScopes.back();
}

void ByteCodeEmitter::emit_defer_region(gmt::Ref<const SyntaxNode> bodyNode)
{
	register_defer();

	std::size_t deferAt = mark_position();
	m_encoder.emit_defer(0);

	std::size_t skipAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(deferAt);
	bodyNode.as_ptr()->accept(*this);
	m_encoder.emit_defer_break();

	patch_jump(skipAt);
}

void ByteCodeEmitter::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	m_deferScopes.push_back(0);
	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
		statement.as_ptr()->accept(*this);

	emit_scope_defers();
}

void ByteCodeEmitter::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr || node->get_expression().is_null())
		return;

	node->get_expression().as_ptr()->accept(*this);

	std::optional<gmt::Ref<const ExpressionSyntax>> expressionRef = ref_from(node->get_expression().as_ptr());
	gmt::Ref<StructSymbol> type = expressionRef.has_value() ? get_model().get_expression_type(*expressionRef) : gmt::nullref;

	bool discardable = !type.is_null() && type != get_symbol_table().get_void_primitive();
	if (discardable)
	{
		m_encoder.emit_pop();
		eval_pop();
	}
}

void ByteCodeEmitter::visit_variable_statement(const VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const VariableStatementSyntax>> nodeRef = ref_from(node);
	gmt::Ref<VariableSymbol> symbol = nodeRef.has_value() ? lookup_symbol<VariableSymbol>(*nodeRef) : gmt::nullref;

	if (symbol.is_null())
	{
		if (!node->get_expression().is_null())
		{
			node->get_expression().as_ptr()->accept(*this);
			eval_pop();
		}

		return;
	}

	std::optional<std::uint16_t> slot = slot_of(symbol);
	if (!slot.has_value())
		slot = allocate_temporary(symbol->get_type(), PlacementVerdict::Heap);

	if (node->get_expression().is_null())
		return;

	PlacementVerdict verdict = local_verdict(symbol);
	gmt::Ref<const ExpressionSyntax> initializer = node->get_expression();

	if (verdict == PlacementVerdict::Scalar)
	{
		unsupported(*nodeRef, L"constructor calls on dematerialized locals");
		return;
	}

	bool stackPlaced = verdict == PlacementVerdict::Stack || verdict == PlacementVerdict::StackReturn;

	if (stackPlaced && initializer->get_kind() == SyntaxKind::ObjectExpression)
	{
		emit_object_construction(initializer.as<ObjectExpressionSyntax>().as_ptr(), *slot);
		return;
	}

	if (stackPlaced && initializer->get_kind() == SyntaxKind::RangeExpression)
	{
		emit_range_cursor(initializer.as_ptr(), *slot);
		return;
	}

	// composite-returning calls write straight into the local's slot through
	// the chained return storage instead of bouncing a pointer off the heap
	if (stackPlaced && initializer->get_kind() == SyntaxKind::InvokationExpression)
	{
		gmt::Ref<MethodSymbol> intoMethod = into_call_method(initializer);
		if (!intoMethod.is_null())
		{
			const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(initializer.as_ptr());
			emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments().as_ptr(), *slot);
			return;
		}
	}

	initializer.as_ptr()->accept(*this);
	m_encoder.emit_store_local(*slot);
	eval_pop();
}

void ByteCodeEmitter::visit_return_statement(const ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	gmt::Ref<const ExpressionSyntax> expression = node->get_expression();

	// tail calls forward the current frame's return slot: the callee writes
	// its result straight into the caller's chained return storage
	gmt::Ref<MethodSymbol> intoMethod = into_call_method(expression);
	if (!expression.is_null() && !intoMethod.is_null())
	{
		const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(expression.as_ptr());
		emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments().as_ptr(), ReturnSlotSentinel);

		emit_all_defers();
		m_encoder.emit_return_placed();
		return;
	}

	if (!expression.is_null())
		expression.as_ptr()->accept(*this);

	emit_all_defers();
	m_encoder.emit_return();

	if (!expression.is_null())
		eval_pop();
}

void ByteCodeEmitter::visit_if_statement(const IfStatementSyntax* node)
{
	if (node == nullptr)
		return;

	emit_conditional_chain(node);
}

void ByteCodeEmitter::visit_unless_statement(const UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	emit_conditional_chain(node);
}

void ByteCodeEmitter::emit_conditional_chain(const ConditionalClauseBaseSyntax* head)
{
	std::vector<std::size_t> endBacktracks;
	const ConditionalClauseBaseSyntax* clause = head;

	while (clause != nullptr)
	{
		SyntaxKind kind = clause->get_kind();
		if (kind == SyntaxKind::ElseStatement)
		{
			if (!clause->get_block().is_null())
				visit_statements_block(clause->get_block().as_ptr());

			break;
		}

		const ConditionalClauseSyntax* conditional = static_cast<const ConditionalClauseSyntax*>(clause);
		if (!conditional->get_condition().is_null())
		{
			conditional->get_condition().as_ptr()->accept(*this);
			eval_pop();
		}

		std::size_t skipAt = mark_position();
		if (kind == SyntaxKind::UnlessStatement)
			m_encoder.emit_jump_true(0);
		else
			m_encoder.emit_jump_false(0);

		if (!conditional->get_block().is_null())
			visit_statements_block(conditional->get_block().as_ptr());

		std::size_t endAt = mark_position();
		m_encoder.emit_jump(0);
		endBacktracks.push_back(endAt);

		patch_jump(skipAt);

		gmt::Ref<const ConditionalClauseBaseSyntax> next = conditional->get_next();
		clause = next.is_null() ? nullptr : next.as_ptr();
	}

	for (std::size_t at : endBacktracks)
		patch_jump(at);
}

void ByteCodeEmitter::visit_while_statement(const WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	loop_scope scope;
	scope.deferScopeDepth = m_deferScopes.size();
	scope.loopStart = mark_position();

	if (!node->get_condition().is_null())
	{
		node->get_condition().as_ptr()->accept(*this);
		eval_pop();
	}

	std::size_t exitAt = mark_position();
	m_encoder.emit_jump_false(0);
	scope.loopEndBacktracks.push_back(exitAt);

	m_loops.push_back(std::move(scope));
	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	loop_scope& active = m_loops.back();
	std::size_t blockEnd = mark_position();
	m_encoder.emit_jump(active.loopStart);
	std::size_t loopEnd = mark_position();

	for (std::size_t at : active.blockEndBacktracks)
		patch_target(at, blockEnd);

	for (std::size_t at : active.loopEndBacktracks)
		patch_target(at, loopEnd);

	m_loops.pop_back();
}

void ByteCodeEmitter::visit_until_statement(const UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	loop_scope scope;
	scope.deferScopeDepth = m_deferScopes.size();
	scope.loopStart = mark_position();

	if (!node->get_condition().is_null())
	{
		node->get_condition().as_ptr()->accept(*this);
		eval_pop();
	}

	std::size_t exitAt = mark_position();
	m_encoder.emit_jump_true(0);
	scope.loopEndBacktracks.push_back(exitAt);

	m_loops.push_back(std::move(scope));
	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	loop_scope& active = m_loops.back();
	std::size_t blockEnd = mark_position();
	m_encoder.emit_jump(active.loopStart);
	std::size_t loopEnd = mark_position();

	for (std::size_t at : active.blockEndBacktracks)
		patch_target(at, blockEnd);

	for (std::size_t at : active.loopEndBacktracks)
		patch_target(at, loopEnd);

	m_loops.pop_back();
}

void ByteCodeEmitter::emit_range_cursor(const ExpressionSyntax* rangeExpression, std::uint16_t slot)
{
	if (rangeExpression->get_kind() != SyntaxKind::RangeExpression)
	{
		// iterate a private copy so the loop never mutates the source value
		rangeExpression->accept(*this);
		m_encoder.emit_store_local(slot);
		eval_pop();
		return;
	}

	SymbolTable& table = get_symbol_table();
	const RangeExpressionSyntax* literal = static_cast<const RangeExpressionSyntax*>(rangeExpression);

	std::uint32_t fromField = field_slot_index(table.get_standard_type(StandardTypes::RangeEnumerable_FromField).as<FieldSymbol>());
	std::uint32_t toField = field_slot_index(table.get_standard_type(StandardTypes::RangeEnumerable_ToField).as<FieldSymbol>());
	std::uint32_t currentField = field_slot_index(table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField).as<FieldSymbol>());

	literal->get_left().as_ptr()->accept(*this);
	std::uint16_t lowerSlot = allocate_temporary(table.get_integer_primitive(), PlacementVerdict::Stack);
	m_encoder.emit_store_local(lowerSlot);
	eval_pop();

	literal->get_right().as_ptr()->accept(*this);
	std::uint16_t upperSlot = allocate_temporary(table.get_integer_primitive(), PlacementVerdict::Stack);
	m_encoder.emit_store_local(upperSlot);
	eval_pop();

	m_encoder.emit_load_slot_addr(slot);
	eval_push(sizeof(void*));
	m_encoder.emit_load_local(lowerSlot);
	eval_push(sizeof(std::int64_t));
	m_encoder.emit_store_field(fromField);
	eval_pop(2);

	m_encoder.emit_load_slot_addr(slot);
	eval_push(sizeof(void*));
	m_encoder.emit_load_local(upperSlot);
	eval_push(sizeof(std::int64_t));

	bool inclusive = literal->get_operator_token().get_type() == TokenType::RangeInclusiveOperator;
	if (inclusive)
	{
		m_encoder.emit_load_const_int8(1);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_math_add();
		eval_pop();
	}

	m_encoder.emit_store_field(toField);
	eval_pop(2);

	m_encoder.emit_load_slot_addr(slot);
	eval_push(sizeof(void*));
	m_encoder.emit_load_local(lowerSlot);
	eval_push(sizeof(std::int64_t));
	m_encoder.emit_load_const_int8(1);
	eval_push(sizeof(std::int64_t));
	m_encoder.emit_math_sub();
	eval_pop();
	m_encoder.emit_store_field(currentField);
	eval_pop(2);
}

void ByteCodeEmitter::visit_for_in_statement(const ForInStatementSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const ForInStatementSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> loopVariable = lookup_symbol<VariableSymbol>(*nodeRef);
	gmt::Ref<StructSymbol> loopVariableType = loopVariable.is_null() ? gmt::nullref : loopVariable->get_type();

	std::optional<std::uint16_t> loopSlot;
	if (!loopVariable.is_null())
		loopSlot = slot_of(loopVariable);

	if (!loopSlot.has_value())
		loopSlot = allocate_temporary(loopVariableType, PlacementVerdict::Heap);

	if (node->get_range_expression().is_null())
		return;

	std::optional<gmt::Ref<const ExpressionSyntax>> rangeRef = ref_from(node->get_range_expression().as_ptr());
	gmt::Ref<StructSymbol> rangeType = rangeRef.has_value() ? get_model().get_expression_type(*rangeRef) : gmt::nullref;

	SymbolTable& table = get_symbol_table();
	bool isLazyRange = !rangeType.is_null() && rangeType == table.get_standard_type(StandardTypes::RangeEnumerable);
	bool isArray = !rangeType.is_null() && rangeType->get_kind() == SyntaxKind::ArrayType;

	gmt::Ref<PropertySymbol> countProperty = gmt::nullref;
	gmt::Ref<IndexatorSymbol> indexator = gmt::nullref;
	bool isIndexable = false;
	if (!isLazyRange && !isArray)
		isIndexable = get_model().resolve_indexable(rangeType, countProperty, indexator);

	std::uint32_t currentField = 0;
	std::uint32_t toField = 0;
	if (isLazyRange)
	{
		currentField = field_slot_index(table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField).as<FieldSymbol>());
		toField = field_slot_index(table.get_standard_type(StandardTypes::RangeEnumerable_ToField).as<FieldSymbol>());
	}

	std::uint16_t rangeSlot = 0;
	std::uint16_t indexSlot = 0;

	if (isLazyRange)
	{
		// the cursor never escapes the loop, so it lives inline in the frame
		rangeSlot = allocate_temporary(rangeType, PlacementVerdict::Stack);
		emit_range_cursor(node->get_range_expression().as_ptr(), rangeSlot);
	}
	else
	{
		gmt::Ref<MethodSymbol> intoMethod = into_call_method(node->get_range_expression());
		if (isIndexable && !intoMethod.is_null())
		{
			// the collection constructs straight into the loop's stack temp
			rangeSlot = allocate_temporary(rangeType, PlacementVerdict::Stack);
			const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(node->get_range_expression().as_ptr());
			emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments().as_ptr(), rangeSlot);
		}
		else
		{
			node->get_range_expression().as_ptr()->accept(*this);

			rangeSlot = allocate_temporary(rangeType, PlacementVerdict::Heap);
			m_encoder.emit_store_local(rangeSlot);
			eval_pop();
		}

		indexSlot = allocate_temporary(table.get_integer_primitive(), PlacementVerdict::Stack);
	}

	loop_scope scope;
	scope.deferScopeDepth = m_deferScopes.size();
	scope.loopStart = mark_position();

	if (isLazyRange)
	{
		// the cursor starts at (lower - 1), so the increment lives at the loop
		// head ahead of the bounds check, mirroring MoveNext semantics
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_field(currentField);
		eval_push(sizeof(std::int64_t));
		eval_pop();
		m_encoder.emit_load_const_int8(1);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_math_add();
		eval_pop();
		m_encoder.emit_store_field(currentField);
		eval_pop(2);

		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_field(currentField);
		eval_push(sizeof(std::int64_t));
		eval_pop();
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_field(toField);
		eval_push(sizeof(std::int64_t));
		eval_pop();
		m_encoder.emit_compare_less();
		eval_pop();
	}
	else if (isIndexable)
	{
		// an interface-typed range dispatches virtually: the abstract accessor
		// carries no body, the runtime resolves the receiver's implementation
		gmt::Ref<MethodSymbol> countGetter = countProperty->get_getter();

		m_encoder.emit_load_local(indexSlot);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		if (is_interface_accessor(countGetter))
			m_encoder.emit_call_interface(countGetter);
		else
			m_encoder.emit_call_method_symbol(countGetter);
		eval_pop();
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_compare_less();
		eval_pop();
	}
	else
	{
		m_encoder.emit_load_local(indexSlot);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_array_length();
		eval_pop();
		m_encoder.emit_compare_less();
		eval_pop();
	}

	std::size_t exitAt = mark_position();
	m_encoder.emit_jump_false(0);
	scope.loopEndBacktracks.push_back(exitAt);

	if (isLazyRange)
	{
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_field(currentField);
		eval_push(sizeof(std::int64_t));
		eval_pop();
	}
	else if (isIndexable)
	{
		gmt::Ref<MethodSymbol> indexGetter = indexator->get_getter();

		m_encoder.emit_load_local(indexSlot);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		if (is_interface_accessor(indexGetter))
			m_encoder.emit_call_interface(indexGetter);
		else
			m_encoder.emit_call_method_symbol(indexGetter);
		eval_pop(2);
		eval_push_type(loopVariableType);
	}
	else
	{
		m_encoder.emit_load_local(rangeSlot);
		eval_push(sizeof(void*));
		m_encoder.emit_load_local(indexSlot);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_load_array_element();
		eval_pop();
	}

	m_encoder.emit_store_local(*loopSlot);
	eval_pop();

	m_loops.push_back(std::move(scope));
	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	loop_scope& active = m_loops.back();
	std::size_t blockEnd = mark_position();

	if (!isLazyRange)
	{
		m_encoder.emit_load_local(indexSlot);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_load_const_int8(1);
		eval_push(sizeof(std::int64_t));
		m_encoder.emit_math_add();
		eval_pop();
		m_encoder.emit_store_local(indexSlot);
		eval_pop();
	}

	m_encoder.emit_jump(active.loopStart);
	std::size_t loopEnd = mark_position();

	for (std::size_t at : active.blockEndBacktracks)
		patch_target(at, blockEnd);

	for (std::size_t at : active.loopEndBacktracks)
		patch_target(at, loopEnd);

	m_loops.pop_back();
}

void ByteCodeEmitter::visit_break_statement(const BreakStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (m_loops.empty())
		return;

	if (node->get_tag().get_type() == TokenType::Identifier)
	{
		std::optional<gmt::Ref<const BreakStatementSyntax>> nodeRef = ref_from(node);
		if (nodeRef.has_value())
			unsupported(*nodeRef, L"labeled break");
		return;
	}

	emit_loop_defers();

	std::size_t at = mark_position();
	m_encoder.emit_jump(0);
	m_loops.back().loopEndBacktracks.push_back(at);
}

void ByteCodeEmitter::visit_continue_statement(const ContinueStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (m_loops.empty())
		return;

	if (node->get_tag().get_type() == TokenType::Identifier)
	{
		std::optional<gmt::Ref<const ContinueStatementSyntax>> nodeRef = ref_from(node);
		if (nodeRef.has_value())
			unsupported(*nodeRef, L"labeled continue");
		return;
	}

	emit_loop_defers();

	std::size_t at = mark_position();
	m_encoder.emit_jump(0);
	m_loops.back().blockEndBacktracks.push_back(at);
}

void ByteCodeEmitter::visit_throw_statement(const ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->get_expression().is_null())
	{
		m_encoder.emit_rethrow();
		return;
	}

	node->get_expression().as_ptr()->accept(*this);
	m_encoder.emit_throw();
	eval_pop();
}

void ByteCodeEmitter::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	std::size_t enterAt = mark_position();
	m_encoder.emit_enter_try(0);

	if (!node->get_try_block().is_null())
		visit_statements_block(node->get_try_block().as_ptr());

	m_encoder.emit_leave_try();

	std::vector<std::size_t> endBacktracks;
	std::size_t endAt = mark_position();
	m_encoder.emit_jump(0);
	endBacktracks.push_back(endAt);

	patch_jump(enterAt);

	gmt::Span<const gmt::Ref<CatchClauseSyntax>> clauses = node->get_catch_clauses();
	if (!clauses.as_span().empty())
		eval_push(sizeof(void*)); // the runtime places the exception on the eval stack

	for (gmt::Ref<CatchClauseSyntax> clauseRef : clauses.as_span())
	{
		const CatchClauseSyntax* clause = clauseRef.as_ptr();

		gmt::Ref<StructSymbol> exceptionType;
		if (!clause->get_exception_type().is_null())
		{
			gmt::Ref<const TypeSyntax> typeNode = clause->get_exception_type();
			gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(typeNode);
			if (!bound.is_null() && bound->is_type())
				exceptionType = bound.as<StructSymbol>();
		}

		std::size_t nextAt = 0;
		bool catchAll = exceptionType.is_null();
		if (!catchAll)
		{
			m_encoder.emit_duplicate();
			eval_push(sizeof(void*));
			m_encoder.emit_is_instance(exceptionType);
			eval_push(sizeof(bool));
			eval_pop(2);

			nextAt = mark_position();
			m_encoder.emit_jump_false(0);
			eval_pop();
		}

		std::optional<gmt::Ref<const CatchClauseSyntax>> clauseNodeRef = ref_from(clause);
		gmt::Ref<VariableSymbol> variable = clauseNodeRef.has_value() ? lookup_symbol<VariableSymbol>(*clauseNodeRef) : gmt::nullref;

		std::optional<std::uint16_t> slot;
		if (!variable.is_null())
			slot = slot_of(variable);

		if (slot.has_value())
		{
			m_encoder.emit_store_local(*slot);
			eval_pop();
		}
		else
		{
			m_encoder.emit_pop();
			eval_pop();
		}

		if (!clause->get_body().is_null())
			visit_statements_block(clause->get_body().as_ptr());

		std::size_t clauseEndAt = mark_position();
		m_encoder.emit_jump(0);
		endBacktracks.push_back(clauseEndAt);

		if (!catchAll)
			patch_jump(nextAt);
	}

	m_encoder.emit_throw();
	eval_pop();

	for (std::size_t at : endBacktracks)
		patch_jump(at);

	m_encoder.emit_end_catch();
}

void ByteCodeEmitter::visit_defer_statement(const DeferStatementSyntax* node)
{
	if (node == nullptr || node->get_expression().is_null())
		return;

	register_defer();

	std::size_t deferAt = mark_position();
	m_encoder.emit_defer(0);

	std::size_t skipAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(deferAt);
	node->get_expression().as_ptr()->accept(*this);

	std::optional<gmt::Ref<const ExpressionSyntax>> expressionRef = ref_from(node->get_expression().as_ptr());
	gmt::Ref<StructSymbol> type = expressionRef.has_value() ? get_model().get_expression_type(*expressionRef) : gmt::nullref;

	bool discardable = !type.is_null() && type != get_symbol_table().get_void_primitive();
	if (discardable)
	{
		m_encoder.emit_pop();
		eval_pop();
	}

	m_encoder.emit_defer_break();
	patch_jump(skipAt);
}

void ByteCodeEmitter::visit_defer_block(const DeferBlockSyntax* node)
{
	if (node == nullptr || node->get_block().is_null())
		return;

	emit_defer_region(node->get_block());
}

void ByteCodeEmitter::visit_defer_variable(const DeferVariableSyntax* node)
{
	if (node == nullptr || node->get_variable().is_null())
		return;

	visit_variable_statement(node->get_variable().as_ptr());

	std::optional<gmt::Ref<const VariableStatementSyntax>> variableRef = ref_from(node->get_variable().as_ptr());
	gmt::Ref<VariableSymbol> symbol = variableRef.has_value() ? lookup_symbol<VariableSymbol>(*variableRef) : gmt::nullref;
	if (symbol.is_null() || symbol->get_type().is_null())
		return;

	gmt::Ref<MethodSymbol> dispose = gmt::nullref;
	for (gmt::Ref<MethodSymbol> candidate : symbol->get_type()->methods)
	{
		if (candidate.is_null() || candidate->get_name() != L"Dispose")
			continue;

		if (!candidate->get_parameters().empty())
			continue;

		dispose = candidate;
		break;
	}

	if (dispose.is_null())
		return;

	std::optional<std::uint16_t> slot = slot_of(symbol);
	if (!slot.has_value())
		return;

	register_defer();

	std::size_t deferAt = mark_position();
	m_encoder.emit_defer(0);

	std::size_t skipAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(deferAt);
	m_encoder.emit_load_local(*slot);
	eval_push(sizeof(void*));
	m_encoder.emit_call_method_symbol(dispose);
	eval_pop();
	m_encoder.emit_defer_break();

	patch_jump(skipAt);
}
