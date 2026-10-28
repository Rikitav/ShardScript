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

#include <gmt/Containers.hpp>

using namespace shard;

namespace
{
	// accessors belong to a property or indexator, so the declaring type sits
	// one level above the accessor's direct parent
	bool is_interface_accessor(MethodSymbol* accessor)
	{
		const SyntaxSymbol* owner = accessor->get_parent();
		if (!(owner == nullptr) && (owner->get_kind() == SyntaxKind::PropertyDeclaration || owner->get_kind() == SyntaxKind::IndexatorDeclaration))
			owner = owner->get_parent();

		return !(owner == nullptr) && owner->get_kind() == SyntaxKind::InterfaceDeclaration;
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

void ByteCodeEmitter::emit_defer_region(const SyntaxNode* bodyNode)
{
	register_defer();

	std::size_t deferAt = mark_position();
	m_encoder.emit_defer(0);

	std::size_t skipAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(deferAt);
	bodyNode->accept(*this);
	m_encoder.emit_defer_break();

	patch_jump(skipAt);
}

void ByteCodeEmitter::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	m_deferScopes.push_back(0);
	for (StatementSyntax* const& statement : node->get_statements())
		statement->accept(*this);

	emit_scope_defers();
}

void ByteCodeEmitter::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr || (node->get_expression() == nullptr))
		return;

	node->get_expression()->accept(*this);

	std::optional<const ExpressionSyntax*> expressionRef = ref_from(node->get_expression());
	StructSymbol* type = expressionRef.has_value() ? get_model().get_expression_type(*expressionRef) : nullptr;

	bool discardable = !(type == nullptr) && type != get_symbol_table().get_void_primitive();
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

	std::optional<const VariableStatementSyntax*> nodeRef = ref_from(node);
	VariableSymbol* symbol = nodeRef.has_value() ? lookup_symbol<VariableSymbol>(*nodeRef) : nullptr;

	if ((symbol == nullptr))
	{
		if (!(node->get_expression() == nullptr))
		{
			node->get_expression()->accept(*this);
			eval_pop();
		}

		return;
	}

	std::optional<std::uint16_t> slot = slot_of(symbol);
	if (!slot.has_value())
		slot = allocate_temporary(symbol->get_type(), PlacementVerdict::Heap);

	if ((node->get_expression() == nullptr))
		return;

	PlacementVerdict verdict = local_verdict(symbol);
	const ExpressionSyntax* initializer = node->get_expression();

	if (verdict == PlacementVerdict::Scalar)
	{
		unsupported(*nodeRef, L"constructor calls on dematerialized locals");
		return;
	}

	bool stackPlaced = verdict == PlacementVerdict::Stack || verdict == PlacementVerdict::StackReturn;

	if (stackPlaced && initializer->get_kind() == SyntaxKind::ObjectExpression)
	{
		emit_object_construction(gmt::as<ObjectExpressionSyntax>(initializer), *slot);
		return;
	}

	if (stackPlaced && initializer->get_kind() == SyntaxKind::RangeExpression)
	{
		emit_range_cursor(initializer, *slot);
		return;
	}

	// composite-returning calls write straight into the local's slot through
	// the chained return storage instead of bouncing a pointer off the heap
	if (stackPlaced && initializer->get_kind() == SyntaxKind::InvokationExpression)
	{
		MethodSymbol* intoMethod = into_call_method(initializer);
		if (!(intoMethod == nullptr))
		{
			const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(initializer);
			emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments(), *slot);
			return;
		}
	}

	initializer->accept(*this);
	m_encoder.emit_store_local(*slot);
	eval_pop();
}

void ByteCodeEmitter::visit_return_statement(const ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	const ExpressionSyntax* expression = node->get_expression();

	// tail calls forward the current frame's return slot: the callee writes
	// its result straight into the caller's chained return storage
	MethodSymbol* intoMethod = into_call_method(expression);
	if (!(expression == nullptr) && !(intoMethod == nullptr))
	{
		const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(expression);
		emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments(), ReturnSlotSentinel);

		emit_all_defers();
		m_encoder.emit_return_placed();
		return;
	}

	if (!(expression == nullptr))
		expression->accept(*this);

	emit_all_defers();
	m_encoder.emit_return();

	if (!(expression == nullptr))
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
	gmt::vector<std::size_t> endBacktracks;
	const ConditionalClauseBaseSyntax* clause = head;

	while (clause != nullptr)
	{
		SyntaxKind kind = clause->get_kind();
		if (kind == SyntaxKind::ElseStatement)
		{
			if (!(clause->get_block() == nullptr))
				visit_statements_block(clause->get_block());

			break;
		}

		const ConditionalClauseSyntax* conditional = static_cast<const ConditionalClauseSyntax*>(clause);
		if (!(conditional->get_condition() == nullptr))
		{
			conditional->get_condition()->accept(*this);
			eval_pop();
		}

		std::size_t skipAt = mark_position();
		if (kind == SyntaxKind::UnlessStatement)
			m_encoder.emit_jump_true(0);
		else
			m_encoder.emit_jump_false(0);

		if (!(conditional->get_block() == nullptr))
			visit_statements_block(conditional->get_block());

		std::size_t endAt = mark_position();
		m_encoder.emit_jump(0);
		endBacktracks.push_back(endAt);

		patch_jump(skipAt);

		const ConditionalClauseBaseSyntax* next = conditional->get_next();
		clause = (next == nullptr) ? nullptr : next;
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

	if (!(node->get_condition() == nullptr))
	{
		node->get_condition()->accept(*this);
		eval_pop();
	}

	std::size_t exitAt = mark_position();
	m_encoder.emit_jump_false(0);
	scope.loopEndBacktracks.push_back(exitAt);

	m_loops.push_back(std::move(scope));
	if (!(node->get_block() == nullptr))
		visit_statements_block(node->get_block());

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

	if (!(node->get_condition() == nullptr))
	{
		node->get_condition()->accept(*this);
		eval_pop();
	}

	std::size_t exitAt = mark_position();
	m_encoder.emit_jump_true(0);
	scope.loopEndBacktracks.push_back(exitAt);

	m_loops.push_back(std::move(scope));
	if (!(node->get_block() == nullptr))
		visit_statements_block(node->get_block());

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

	std::uint32_t fromField = field_slot_index(static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::RangeEnumerable_FromField)));
	std::uint32_t toField = field_slot_index(static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::RangeEnumerable_ToField)));
	std::uint32_t currentField = field_slot_index(static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField)));

	literal->get_left()->accept(*this);
	std::uint16_t lowerSlot = allocate_temporary(table.get_integer_primitive(), PlacementVerdict::Stack);
	m_encoder.emit_store_local(lowerSlot);
	eval_pop();

	literal->get_right()->accept(*this);
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

	std::optional<const ForInStatementSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	VariableSymbol* loopVariable = lookup_symbol<VariableSymbol>(*nodeRef);
	StructSymbol* loopVariableType = (loopVariable == nullptr) ? nullptr : loopVariable->get_type();

	std::optional<std::uint16_t> loopSlot;
	if (!(loopVariable == nullptr))
		loopSlot = slot_of(loopVariable);

	if (!loopSlot.has_value())
		loopSlot = allocate_temporary(loopVariableType, PlacementVerdict::Heap);

	if ((node->get_range_expression() == nullptr))
		return;

	std::optional<const ExpressionSyntax*> rangeRef = ref_from(node->get_range_expression());
	StructSymbol* rangeType = rangeRef.has_value() ? get_model().get_expression_type(*rangeRef) : nullptr;

	SymbolTable& table = get_symbol_table();
	bool isLazyRange = !(rangeType == nullptr) && rangeType == table.get_standard_type(StandardTypes::RangeEnumerable);
	bool isArray = !(rangeType == nullptr) && rangeType->get_kind() == SyntaxKind::ArrayType;

	PropertySymbol* countProperty = nullptr;
	IndexatorSymbol* indexator = nullptr;
	bool isIndexable = false;
	if (!isLazyRange && !isArray)
		isIndexable = get_model().resolve_indexable(rangeType, countProperty, indexator);

	std::uint32_t currentField = 0;
	std::uint32_t toField = 0;
	if (isLazyRange)
	{
		currentField = field_slot_index(static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::RangeEnumerable_CurrentField)));
		toField = field_slot_index(static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::RangeEnumerable_ToField)));
	}

	std::uint16_t rangeSlot = 0;
	std::uint16_t indexSlot = 0;

	if (isLazyRange)
	{
		// the cursor never escapes the loop, so it lives inline in the frame
		rangeSlot = allocate_temporary(rangeType, PlacementVerdict::Stack);
		emit_range_cursor(node->get_range_expression(), rangeSlot);
	}
	else
	{
		MethodSymbol* intoMethod = into_call_method(node->get_range_expression());
		if (isIndexable && !(intoMethod == nullptr))
		{
			// the collection constructs straight into the loop's stack temp
			rangeSlot = allocate_temporary(rangeType, PlacementVerdict::Stack);
			const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(node->get_range_expression());
			emit_method_call_into(intoMethod, invokation->get_previous(), invokation->get_arguments(), rangeSlot);
		}
		else
		{
			node->get_range_expression()->accept(*this);

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
		MethodSymbol* countGetter = countProperty->get_getter();

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
		MethodSymbol* indexGetter = indexator->get_getter();

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
	if (!(node->get_block() == nullptr))
		visit_statements_block(node->get_block());

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
		std::optional<const BreakStatementSyntax*> nodeRef = ref_from(node);
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
		std::optional<const ContinueStatementSyntax*> nodeRef = ref_from(node);
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

	if ((node->get_expression() == nullptr))
	{
		m_encoder.emit_rethrow();
		return;
	}

	node->get_expression()->accept(*this);
	m_encoder.emit_throw();
	eval_pop();
}

void ByteCodeEmitter::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	std::size_t enterAt = mark_position();
	m_encoder.emit_enter_try(0);

	if (!(node->get_try_block() == nullptr))
		visit_statements_block(node->get_try_block());

	m_encoder.emit_leave_try();

	gmt::vector<std::size_t> endBacktracks;
	std::size_t endAt = mark_position();
	m_encoder.emit_jump(0);
	endBacktracks.push_back(endAt);

	patch_jump(enterAt);

	std::span<CatchClauseSyntax* const> clauses = node->get_catch_clauses();
	if (!clauses.empty())
		eval_push(sizeof(void*)); // the runtime places the exception on the eval stack

	for (CatchClauseSyntax* clauseRef : clauses)
	{
		const CatchClauseSyntax* clause = clauseRef;

		StructSymbol* exceptionType = nullptr;
		if (!(clause->get_exception_type() == nullptr))
		{
			const TypeSyntax* typeNode = clause->get_exception_type();
			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(typeNode);
			if (!(bound == nullptr) && bound->is_type())
				exceptionType = static_cast<StructSymbol*>(bound);
		}

		std::size_t nextAt = 0;
		bool catchAll = (exceptionType == nullptr);
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

		std::optional<const CatchClauseSyntax*> clauseNodeRef = ref_from(clause);
		VariableSymbol* variable = clauseNodeRef.has_value() ? lookup_symbol<VariableSymbol>(*clauseNodeRef) : nullptr;

		std::optional<std::uint16_t> slot;
		if (!(variable == nullptr))
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

		if (!(clause->get_body() == nullptr))
			visit_statements_block(clause->get_body());

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
	if (node == nullptr || (node->get_expression() == nullptr))
		return;

	register_defer();

	std::size_t deferAt = mark_position();
	m_encoder.emit_defer(0);

	std::size_t skipAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(deferAt);
	node->get_expression()->accept(*this);

	std::optional<const ExpressionSyntax*> expressionRef = ref_from(node->get_expression());
	StructSymbol* type = expressionRef.has_value() ? get_model().get_expression_type(*expressionRef) : nullptr;

	bool discardable = !(type == nullptr) && type != get_symbol_table().get_void_primitive();
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
	if (node == nullptr || (node->get_block() == nullptr))
		return;

	emit_defer_region(node->get_block());
}

void ByteCodeEmitter::visit_defer_variable(const DeferVariableSyntax* node)
{
	if (node == nullptr || (node->get_variable() == nullptr))
		return;

	visit_variable_statement(node->get_variable());

	std::optional<const VariableStatementSyntax*> variableRef = ref_from(node->get_variable());
	VariableSymbol* symbol = variableRef.has_value() ? lookup_symbol<VariableSymbol>(*variableRef) : nullptr;
	if ((symbol == nullptr) || (symbol->get_type() == nullptr))
		return;

	MethodSymbol* dispose = nullptr;
	for (MethodSymbol* candidate : symbol->get_type()->methods)
	{
		if ((candidate == nullptr) || candidate->get_name() != L"Dispose")
			continue;

		if (!candidate->get_parameters().empty())
			continue;

		dispose = candidate;
		break;
	}

	if ((dispose == nullptr))
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
