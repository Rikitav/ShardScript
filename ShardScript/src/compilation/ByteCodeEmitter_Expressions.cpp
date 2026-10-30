#include <shard/compilation/ByteCodeEmitter.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>

#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericMethodSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Containers.hpp>

#include <cstdlib>
#include <cwchar>
#include <iostream>

using namespace shard;

namespace
{
	shard::DelegateTypeSymbol* unwrap_delegate_type(shard::StructSymbol* type)
	{
		if (type == nullptr)
			return nullptr;

		if (type->get_kind() == shard::SyntaxKind::DelegateType)
			return static_cast<shard::DelegateTypeSymbol*>(type);

		if (type->get_kind() == shard::SyntaxKind::GenericType)
		{
			shard::StructSymbol* underlaying = static_cast<shard::GenericTypeSymbol*>(type)->get_underlaying_type();
			if (!(underlaying == nullptr) && underlaying->get_kind() == shard::SyntaxKind::DelegateType)
				return static_cast<shard::DelegateTypeSymbol*>(underlaying);
		}

		return nullptr;
	}

	class lambda_capture_checker final : public SemanticVisitor
	{
		StructSymbol* m_captureBox = nullptr;
		bool m_captured = false;

		void check_bound(const SyntaxNode* node, const ExpressionSyntax* previous)
		{
			// variable and parameter captures resolve to capture box fields at
			// emission. Only implicit-'this' members are unsupported: anything
			// with an explicit receiver expression stays legal
			if (!(previous == nullptr))
				return;

			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node);
			if ((bound == nullptr) || !bound->is_member())
				return;

			if (static_cast<MemberSymbol*>(bound)->get_linking() == LINK_STATIC)
				return;

			const SyntaxSymbol* parent = bound->get_parent();
			if ((parent == nullptr) || !parent->is_type())
				return;

			if (parent != m_captureBox)
				m_captured = true;
		}

	public:
		lambda_capture_checker(SemanticModel& model, DiagnosticsContext& diagnostics, StructSymbol* captureBox)
			: SemanticVisitor(model, diagnostics)
			, m_captureBox(captureBox)
		{ }

		bool get_captured() const
		{
			return m_captured;
		}

		void visit_member_access_expression(const MemberAccessExpressionSyntax* node) override
		{
			std::optional<const MemberAccessExpressionSyntax*> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				check_bound(*nodeRef, node->get_previous());

			SyntaxVisitor::visit_member_access_expression(node);
		}

		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override
		{
			std::optional<const IdentifierNameTypeSyntax*> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				check_bound(*nodeRef, nullptr);

			SyntaxVisitor::visit_identifier_name_type(node);
		}

		void visit_invokation_expression(const InvokationExpressionSyntax* node) override
		{
			std::optional<const InvokationExpressionSyntax*> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				check_bound(*nodeRef, node->get_previous());

			SyntaxVisitor::visit_invokation_expression(node);
		}
	};

	std::int64_t parse_integer(std::wstring_view lexeme)
	{
		return std::wcstoll(gmt::wstring(lexeme).c_str(), nullptr, 0);
	}

	double parse_double(std::wstring_view lexeme)
	{
		return std::wcstod(gmt::wstring(lexeme).c_str(), nullptr);
	}

	wchar_t parse_char_literal(std::wstring_view lexeme)
	{
		// the lexer stores the literal content already unescaped, without quotes
		if (lexeme.empty())
			return L'\0';

		return lexeme[0];
	}

	bool is_assignment_token(TokenType type)
	{
		switch (type)
		{
			case TokenType::AssignOperator:
			case TokenType::AddAssignOperator:
			case TokenType::SubAssignOperator:
			case TokenType::MultAssignOperator:
			case TokenType::DivAssignOperator:
			case TokenType::ModAssignOperator:
			case TokenType::PowAssignOperator:
				return true;

			default:
				return false;
		}
	}

	TokenType compound_arithmetic(TokenType type)
	{
		switch (type)
		{
			case TokenType::AddAssignOperator: return TokenType::AddOperator;
			case TokenType::SubAssignOperator: return TokenType::SubOperator;
			case TokenType::MultAssignOperator: return TokenType::MultOperator;
			case TokenType::DivAssignOperator: return TokenType::DivOperator;
			case TokenType::ModAssignOperator: return TokenType::ModOperator;
			case TokenType::PowAssignOperator: return TokenType::PowOperator;
			default: return TokenType::Unknown;
		}
	}
}

void ByteCodeEmitter::visit_literal_expression(const LiteralExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	SyntaxToken literal = node->get_literal();
	//SymbolTable& table = get_symbol_table();

	switch (literal.get_type())
	{
		case TokenType::NullLiteral:
		{
			m_encoder.emit_load_const_null();
			eval_push(sizeof(void*));
			return;
		}

		case TokenType::BooleanLiteral:
		{
			m_encoder.emit_load_const_bool(literal.get_lexeme() == L"true");
			eval_push(sizeof(bool));
			return;
		}

		case TokenType::NumberLiteral:
		{
			m_encoder.emit_load_const_int64(parse_integer(literal.get_lexeme()));
			eval_push(sizeof(std::int64_t));
			return;
		}

		case TokenType::DoubleLiteral:
		{
			// the binder already parsed and validated the value locale-independently;
			// re-parsing the lexeme here would be sensitive to the process locale
			double value = 0.0;
			std::optional<const LiteralExpressionSyntax*> nodeRef = ref_from(node);
			LiteralSymbol* symbol = nodeRef.has_value() ? lookup_symbol<LiteralSymbol>(*nodeRef) : nullptr;

			if (!(symbol == nullptr))
				value = symbol->get_double_value();
			else
				value = parse_double(literal.get_lexeme());

			m_encoder.emit_load_const_double64(value);
			eval_push(sizeof(double));
			return;
		}

		case TokenType::CharLiteral:
		{
			m_encoder.emit_load_const_char16(parse_char_literal(literal.get_lexeme()));
			eval_push(sizeof(wchar_t));
			return;
		}

		case TokenType::StringLiteral:
		{
			gmt::wstring text(literal.get_lexeme());
			m_encoder.emit_load_const_string(text.c_str());
			eval_push(sizeof(void*));
			return;
		}

		default:
		{
			m_encoder.emit_load_const_null();
			eval_push(sizeof(void*));
			return;
		}
	}
}

void ByteCodeEmitter::visit_binary_expression(const BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	TokenType op = node->get_operator_token().get_type();
	if (is_assignment_token(op))
	{
		if (op == TokenType::AssignOperator)
			emit_assignment(node);
		else
			emit_compound_assignment(node, compound_arithmetic(op));

		return;
	}

	if (op == TokenType::AndOperator || op == TokenType::OrOperator)
	{
		node->get_left()->accept(*this);
		m_encoder.emit_duplicate();
		eval_push(sizeof(bool));

		std::size_t endAt = mark_position();
		if (op == TokenType::AndOperator)
			m_encoder.emit_jump_false(0);
		else
			m_encoder.emit_jump_true(0);

		eval_pop();
		m_encoder.emit_pop();
		eval_pop();

		node->get_right()->accept(*this);
		patch_jump(endAt);
		return;
	}

	std::optional<const BinaryExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
	{
		OperatorSymbol* overload = lookup_symbol<OperatorSymbol>(*nodeRef);
		if (!(overload == nullptr))
		{
			StructSymbol* receiverType = get_model().get_expression_type(node->get_left());
			MethodSymbol* target = resolve_call_target(*nodeRef, overload, receiverType);
			if (target == nullptr)
				return;

			node->get_right()->accept(*this);
			node->get_left()->accept(*this);

			m_encoder.emit_call_method_symbol(target);
			eval_pop(2);
			eval_push_type(target->get_return_type());
			return;
		}
	}

	node->get_left()->accept(*this);
	node->get_right()->accept(*this);

	switch (op)
	{
		case TokenType::EqualsOperator: m_encoder.emit_compare_equal(); break;
		case TokenType::NotEqualsOperator: m_encoder.emit_compare_not_equal(); break;
		case TokenType::GreaterOperator: m_encoder.emit_compare_greater(); break;
		case TokenType::GreaterOrEqualsOperator: m_encoder.emit_compare_greater_or_equal(); break;
		case TokenType::LessOperator: m_encoder.emit_compare_less(); break;
		case TokenType::LessOrEqualsOperator: m_encoder.emit_compare_less_or_equal(); break;

		default:
		{
			if (!emit_arithmetic_op(op))
			{
				unsupported(*nodeRef, L"bitwise operators");
				m_encoder.emit_pop();
				eval_pop();
				return;
			}

			break;
		}
	}

	eval_pop();
}

bool ByteCodeEmitter::emit_arithmetic_op(TokenType type)
{
	switch (type)
	{
		case TokenType::AddOperator: m_encoder.emit_math_add(); return true;
		case TokenType::SubOperator: m_encoder.emit_math_sub(); return true;
		case TokenType::MultOperator: m_encoder.emit_math_mult(); return true;
		case TokenType::DivOperator: m_encoder.emit_math_div(); return true;
		case TokenType::ModOperator: m_encoder.emit_math_mod(); return true;
		case TokenType::PowOperator: m_encoder.emit_math_pow(); return true;
		case TokenType::LeftShiftOperator: m_encoder.emit_math_shl(); return true;
		case TokenType::RightShiftOperator: m_encoder.emit_math_shr(); return true;
		default: return false;
	}
}

void ByteCodeEmitter::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	TokenType op = node->get_operator_token().get_type();
	std::optional<const UnaryExpressionSyntax*> nodeRef = ref_from(node);

	if (op == TokenType::TryKeyword)
	{
		node->get_operand()->accept(*this);
		return;
	}

	if (op == TokenType::NotOperator)
	{
		node->get_operand()->accept(*this);
		m_encoder.emit_logical_not();
		return;
	}

	if (op == TokenType::SubOperator)
	{
		node->get_operand()->accept(*this);
		m_encoder.emit_math_negative();
		return;
	}

	if (op == TokenType::IncrementOperator || op == TokenType::DecrementOperator)
	{
		bool increment = op == TokenType::IncrementOperator;
		const ExpressionSyntax* operand = node->get_operand();

		if (!is_identifier_target(operand))
		{
			unsupported(*nodeRef, L"increment of non-variable targets");
			return;
		}

		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node->get_operand());

		std::optional<std::uint16_t> slot;
		if (!(bound == nullptr) && (bound->get_kind() == SyntaxKind::VariableStatement || bound->get_kind() == SyntaxKind::Parameter))
			slot = slot_of(bound);

		if (!slot.has_value())
		{
			unsupported(*nodeRef, L"increment of non-variable targets");
			return;
		}

		bool postfix = node->get_is_postfix();
		m_encoder.emit_load_local(*slot);
		eval_push(sizeof(std::int64_t));

		if (postfix)
		{
			m_encoder.emit_duplicate();
			eval_push(sizeof(std::int64_t));
		}

		m_encoder.emit_load_const_int8(1);
		eval_push(sizeof(std::int64_t));

		if (increment)
			m_encoder.emit_math_add();
		else
			m_encoder.emit_math_sub();

		eval_pop();

		if (!postfix)
		{
			m_encoder.emit_duplicate();
			eval_push(sizeof(std::int64_t));
		}

		m_encoder.emit_store_local(*slot);
		eval_pop();
		return;
	}

	unsupported(*nodeRef, L"this unary operator");
}

void ByteCodeEmitter::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const MemberAccessExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	emit_identifier_load(*nodeRef, node->get_previous());
}

void ByteCodeEmitter::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const IdentifierNameTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	emit_identifier_load(*nodeRef, nullptr);
}

bool ByteCodeEmitter::is_identifier_target(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return false;

	SyntaxKind kind = expression->get_kind();
	return kind == SyntaxKind::MemberAccessExpression || kind == SyntaxKind::IdentifierNameType;
}

void ByteCodeEmitter::emit_identifier_load(const SyntaxNode* node, const ExpressionSyntax* previous)
{
	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node);
	if (bound == nullptr)
	{
		if (!(previous == nullptr))
			return;

		return;
	}

	if (bound->is_type())
		return;

	SyntaxKind kind = bound->get_kind();
	switch (kind)
	{
		case SyntaxKind::VariableStatement:
		case SyntaxKind::Parameter:
		{
			std::optional<std::uint16_t> slot = slot_of(bound);
			if (slot.has_value())
			{
				m_encoder.emit_load_local(*slot);
				eval_push(sizeof(void*));
				return;
			}

			// the slot is absent from this method's frame: the reference
			// belongs to an enclosing method and resolves to a capture box field
			FieldSymbol* captureField = capture_field_of(bound);
			if (!(captureField == nullptr))
			{
				emit_instance_field_load(captureField, nullptr);
				return;
			}

			unsupported(node, L"lambda captures");
			return;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* field = static_cast<FieldSymbol*>(bound);
			if (field->get_is_enum_value())
			{
				m_encoder.emit_load_enum_field(field);
				eval_push(sizeof(std::int64_t));
				return;
			}

			if (field->get_linking() == LINK_STATIC)
			{
				m_encoder.emit_load_static_field(field);
				eval_push(sizeof(void*));
				return;
			}

			emit_instance_field_load(field, previous);
			return;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* property = static_cast<PropertySymbol*>(bound);
			MethodSymbol* getter = property->get_getter();

			if (getter == nullptr)
			{
				unsupported(node, L"property without a getter");
				return;
			}

			StructSymbol* receiverType = (previous == nullptr) ? nullptr : get_model().get_expression_type(previous);
			getter = resolve_call_target(node, getter, receiverType);
			if (getter == nullptr)
				return;

			if (!(previous == nullptr))
				previous->accept(*this);
			else
			{
				m_encoder.emit_load_local(0);
				eval_push(sizeof(void*));
			}

			m_encoder.emit_call_method_symbol(getter);
			eval_pop();

			// the binder's expression type carries the substituted value type;
			// the open declared type can still be a type parameter
			StructSymbol* valueType = get_model().get_expression_type(static_cast<const ExpressionSyntax*>(node));
			eval_push_type((valueType == nullptr) ? getter->get_return_type() : valueType);
			return;
		}

		default:
		{
			unsupported(node, L"this member access shape");
			return;
		}
	}
}

void ByteCodeEmitter::emit_instance_field_load(FieldSymbol* field, const ExpressionSyntax* previous)
{
	// scalar locals are dematerialized: the field is a frame slot of its own
	if (!(previous == nullptr) && is_identifier_target(previous))
	{
		SyntaxSymbol* previousBound = get_symbol_table().lookup_symbol(previous);
		if (!(previousBound == nullptr) && local_verdict(previousBound) == PlacementVerdict::Scalar)
		{
			std::optional<std::uint16_t> scalarSlot = scalar_field_slot(previousBound, field);
			if (scalarSlot.has_value())
			{
				m_encoder.emit_load_local(*scalarSlot);
				eval_push(payload_of(field->get_return_type()));
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

	m_encoder.emit_load_field(field_slot_index(field));
	eval_push(payload_of(field->get_return_type()));
	eval_pop();
}

FieldSymbol* ByteCodeEmitter::capture_field_of(SyntaxSymbol* bound) const
{
	if (m_generatingFor == nullptr)
		return nullptr;

	const gmt::vector<SyntaxSymbol*>* captures = m_pragmatic.get_lambda_captures(m_generatingFor);
	if (captures == nullptr)
		return nullptr;

	const SyntaxSymbol* parent = m_generatingFor->get_parent();
	if ((parent == nullptr) || !parent->is_type())
		return nullptr;

	StructSymbol* captureBox = gmt::as<StructSymbol>(parent);
	for (std::size_t i = 0; i < captures->size() && i < captureBox->fields.size(); ++i)
	{
		if ((*captures)[i] == bound)
			return captureBox->fields[i];
	}

	return nullptr;
}

void ByteCodeEmitter::emit_arguments_reversed(const ArgumentsListSyntax* arguments)
{
	if (arguments == nullptr)
		return;

	std::span<ArgumentSyntax* const> list = arguments->get_arguments();
	for (std::size_t i = list.size(); i-- > 0;)
	{
		const ArgumentSyntax* argument = list[i];
		if ((argument->get_expression() == nullptr))
			continue;

		argument->get_expression()->accept(*this);
	}
}

bool ByteCodeEmitter::is_extension_call(MethodSymbol* method, const ExpressionSyntax* receiver, const ArgumentsListSyntax* arguments)
{
	if ((receiver == nullptr) || method->get_linking() != LINK_STATIC)
		return false;

	// the binder matches scope functions against the receiver prepended to
	// the argument list, so a bound static callee with one more parameter
	// than explicit arguments consumes the receiver as its first parameter
	StructSymbol* receiverType = get_model().get_expression_type(receiver);
	if (receiverType == nullptr)
		return false;

	std::size_t explicitArguments = arguments == nullptr ? 0 : arguments->get_arguments().size();
	return method->get_parameters().size() == explicitArguments + 1;
}

void ByteCodeEmitter::emit_method_call(MethodSymbol* method, const ExpressionSyntax* receiver, const ArgumentsListSyntax* arguments, [[maybe_unused]] bool receiverFirst)
{
	bool isStatic = method->get_linking() == LINK_STATIC;
	bool extension = is_extension_call(method, receiver, arguments);

	const SyntaxSymbol* parent = method->get_parent();
	bool interfaceDispatch = !(parent == nullptr) && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

	emit_arguments_reversed(arguments);

	std::size_t receiverEval = 0;
	if (!isStatic || extension)
	{
		if (!(receiver == nullptr))
			receiver->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		receiverEval = 1;
	}

	if (interfaceDispatch)
		m_encoder.emit_call_interface(method);
	else
		m_encoder.emit_call_method_symbol(method);

	// the extension receiver fills the first parameter slot, so the parameter
	// count already covers it
	std::size_t argumentCount = method->get_parameters().size() + (extension ? 0 : receiverEval);
	eval_pop(argumentCount);

	bool isAsync = method->get_is_async();
	StructSymbol* resultType = method->get_return_type();
	if (isAsync)
		resultType = get_model().get_async_call_type(resultType);

	if (isAsync || (!(resultType == nullptr) && resultType != get_symbol_table().get_void_primitive()))
		eval_push_type(resultType);
}

void ByteCodeEmitter::emit_method_call_into(MethodSymbol* method, const ExpressionSyntax* receiver, const ArgumentsListSyntax* arguments, std::uint16_t slot)
{
	bool isStatic = method->get_linking() == LINK_STATIC;
	bool extension = is_extension_call(method, receiver, arguments);

	emit_arguments_reversed(arguments);

	std::size_t receiverEval = 0;
	if (!isStatic || extension)
	{
		if (!(receiver == nullptr))
			receiver->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		receiverEval = 1;
	}

	m_encoder.emit_call_method_into(method, slot);
	eval_pop(method->get_parameters().size() + (extension ? 0 : receiverEval));
}

MethodSymbol* ByteCodeEmitter::into_call_method(const ExpressionSyntax* expression)
{
	if ((expression == nullptr) || expression->get_kind() != SyntaxKind::InvokationExpression)
		return nullptr;

	const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(expression);
	std::optional<const InvokationExpressionSyntax*> invokationRef = ref_from(invokation);
	if (!invokationRef.has_value())
		return nullptr;

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*invokationRef);
	if ((bound == nullptr) || !bound->is_method())
		return nullptr;

	MethodSymbol* method = static_cast<MethodSymbol*>(bound);

	StructSymbol* receiverType = nullptr;
	if (!(invokation->get_previous() == nullptr))
		receiverType = get_model().get_expression_type(invokation->get_previous());

	method = resolve_call_target(*invokationRef, method, receiverType);
	if (method == nullptr)
		return nullptr;

	const SyntaxSymbol* parent = method->get_parent();
	if (!(parent == nullptr) && parent->get_kind() == SyntaxKind::InterfaceDeclaration)
		return nullptr;

	StructSymbol* returnType = method->get_return_type();
	if (returnType == nullptr)
		return nullptr;

	// async calls hand back a Task wrapper, never the plain declared result,
	// so they can never be written into a caller slot of the plain type
	if (method->get_is_async())
		return nullptr;

	SyntaxKind returnKind = returnType->get_kind();
	if (returnKind != SyntaxKind::StructDeclaration && returnKind != SyntaxKind::GenericType)
		return nullptr;

	return method;
}

void ByteCodeEmitter::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const InvokationExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// direct callee expression form: (callee)(arguments)
	if (node->get_identifier().get_type() == TokenType::Unknown && !(node->get_previous() == nullptr))
	{
		StructSymbol* calleeType = get_model().get_expression_type(node->get_previous());
		DelegateTypeSymbol* delegateType = unwrap_delegate_type(calleeType);

		if (delegateType == nullptr)
		{
			unsupported(*nodeRef, L"invokation of a non-delegate expression");
			return;
		}

		GenericTypeSymbol* instantiation = calleeType->get_kind() == SyntaxKind::GenericType
			? static_cast<GenericTypeSymbol*>(calleeType)
			: nullptr;

		emit_arguments_reversed(node->get_arguments());
		node->get_previous()->accept(*this);

		m_encoder.emit_call_delegate(delegateType);

		std::size_t argumentCount = delegateType->get_parameters().size() + 1;
		eval_pop(argumentCount);
		eval_push_type(get_model().substitute_type(delegateType->get_return_type(), instantiation));
		return;
	}

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (bound == nullptr)
		return;

	if (bound->is_method())
	{
		MethodSymbol* method = static_cast<MethodSymbol*>(bound);

		StructSymbol* receiverType = nullptr;
		if (!(node->get_previous() == nullptr))
			receiverType = get_model().get_expression_type(node->get_previous());

		method = resolve_call_target(*nodeRef, method, receiverType);
		if (method == nullptr)
			return;

		emit_method_call(method, node->get_previous(), node->get_arguments(), false);
		return;
	}

	// delegate invocation through a variable, parameter or field
	SyntaxKind kind = bound->get_kind();
	StructSymbol* holderType = nullptr;

	if (kind == SyntaxKind::VariableStatement)
		holderType = static_cast<VariableSymbol*>(bound)->get_type();
	else if (kind == SyntaxKind::Parameter)
		holderType = static_cast<ParameterSymbol*>(bound)->get_type();
	else if (kind == SyntaxKind::FieldDeclaration)
		holderType = static_cast<FieldSymbol*>(bound)->get_return_type();

	DelegateTypeSymbol* delegateType = unwrap_delegate_type(holderType);

	if (delegateType == nullptr)
	{
		unsupported(*nodeRef, L"this invokation shape");
		return;
	}

	// the callee value loads like a member access over the same name, on top
	emit_arguments_reversed(node->get_arguments());

	if (kind == SyntaxKind::FieldDeclaration)
	{
		FieldSymbol* field = static_cast<FieldSymbol*>(bound);
		if (field->get_linking() == LINK_STATIC)
		{
			m_encoder.emit_load_static_field(field);
			eval_push(sizeof(void*));
		}
		else
		{
			if (!(node->get_previous() == nullptr))
				node->get_previous()->accept(*this);
			else
			{
				m_encoder.emit_load_local(0);
				eval_push(sizeof(void*));
			}

			m_encoder.emit_load_field(field_slot_index(field));
			eval_push(sizeof(void*));
			eval_pop();
		}
	}
	else
	{
		std::optional<std::uint16_t> slot = slot_of(bound);
		if (!slot.has_value())
		{
			unsupported(*nodeRef, L"unresolved delegate callee");
			return;
		}

		m_encoder.emit_load_local(*slot);
		eval_push(sizeof(void*));
	}

	m_encoder.emit_call_delegate(delegateType);

	GenericTypeSymbol* instantiation = !(holderType == nullptr) && holderType->get_kind() == SyntaxKind::GenericType
		? static_cast<GenericTypeSymbol*>(holderType)
		: nullptr;

	std::size_t argumentCount = delegateType->get_parameters().size() + 1;
	eval_pop(argumentCount);
	eval_push_type(get_model().substitute_type(delegateType->get_return_type(), instantiation));
}

void ByteCodeEmitter::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const IndexatorExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	IndexatorSymbol* indexator = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (!(indexator == nullptr))
	{
		MethodSymbol* getter = indexator->get_getter();
		if (getter == nullptr)
		{
			unsupported(*nodeRef, L"indexator without a getter");
			return;
		}

		StructSymbol* receiverType = (node->get_previous() == nullptr) ? nullptr : get_model().get_expression_type(node->get_previous());
		getter = resolve_call_target(*nodeRef, getter, receiverType);
		if (getter == nullptr)
			return;

		emit_arguments_reversed(node->get_arguments());
		if (!(node->get_previous() == nullptr))
			node->get_previous()->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		m_encoder.emit_call_method_symbol(getter);

		std::size_t argumentCount = indexator->get_parameters().size() + 1;
		eval_pop(argumentCount);

		StructSymbol* valueType = get_model().get_expression_type(*nodeRef);
		eval_push_type((valueType == nullptr) ? getter->get_return_type() : valueType);
		return;
	}

	// array element access
	node->get_previous()->accept(*this);

	std::span<ArgumentSyntax* const> arguments = node->get_arguments()->get_arguments();
	if (!arguments.empty())
		arguments.front()->get_expression()->accept(*this);

	m_encoder.emit_load_array_element();
	eval_pop();
}

void ByteCodeEmitter::visit_is_expression(const IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	StructSymbol* targetType = nullptr;
	if (!(node->get_target_type() == nullptr))
	{
		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node->get_target_type());
		if (!(bound == nullptr) && bound->is_type())
			targetType = static_cast<StructSymbol*>(bound);
	}

	targetType = substitute_type(targetType);

	node->get_expression()->accept(*this);
	m_encoder.emit_is_instance(targetType);
	eval_push(sizeof(bool));
	eval_pop(2);
}

void ByteCodeEmitter::visit_cast_expression(const CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	StructSymbol* targetType = nullptr;
	if (!(node->get_target_type() == nullptr))
	{
		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node->get_target_type());
		if (!(bound == nullptr) && bound->is_type())
			targetType = static_cast<StructSymbol*>(bound);
	}

	targetType = substitute_type(targetType);

	node->get_expression()->accept(*this);

	if (!(targetType == nullptr) && get_model().is_primitive_type(targetType))
		m_encoder.emit_cast_primitive(targetType);
	else if (node->get_operator_token().get_type() == TokenType::AsOperator)
		m_encoder.emit_cast_as(targetType);
	else
		m_encoder.emit_cast(targetType);

	eval_push(sizeof(void*));
	eval_pop(2);
}

void ByteCodeEmitter::visit_if_expression(const IfExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	node->get_condition()->accept(*this);
	eval_pop();

	std::size_t elseAt = mark_position();
	m_encoder.emit_jump_false(0);

	std::int32_t armDepth = m_evalDepth;
	node->get_then_expression()->accept(*this);

	std::size_t endAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(elseAt);
	m_evalDepth = armDepth;

	node->get_else_expression()->accept(*this);
	patch_jump(endAt);
}

void ByteCodeEmitter::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr || node->get_expression() == nullptr)
		return;

	node->get_expression()->accept(*this);

	m_encoder.emit_await();
	eval_pop();

	StructSymbol* resultType = nullptr;
	std::optional<const AwaitExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		resultType = get_model().get_expression_type(*nodeRef);

	if (!(resultType == nullptr) && resultType != get_symbol_table().get_void_primitive())
		eval_push_type(resultType);
}

void ByteCodeEmitter::visit_range_expression(const RangeExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	node->get_left()->accept(*this);
	node->get_right()->accept(*this);

	bool inclusive = node->get_operator_token().get_type() == TokenType::RangeInclusiveOperator;
	m_encoder.emit_load_const_bool(inclusive);
	eval_push(sizeof(bool));

	StructSymbol* rangeType = static_cast<StructSymbol*>(get_symbol_table().get_standard_type(StandardTypes::RangeEnumerable));
	m_encoder.emit_create_range(rangeType);
	eval_pop(3);
	eval_push(sizeof(void*));
}

void ByteCodeEmitter::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const CollectionExpressionSyntax*> nodeRef = ref_from(node);
	ArrayTypeSymbol* boundArray = nodeRef.has_value() ? lookup_symbol<ArrayTypeSymbol>(*nodeRef) : nullptr;

	StructSymbol* elementType = (boundArray == nullptr) ? nullptr : boundArray->get_underlaying_type();
	elementType = substitute_type(elementType);
	if (elementType == nullptr)
	{
		unsupported(*nodeRef, L"collection expression without a bound element type");
		return;
	}

	std::span<ExpressionSyntax* const> values = node->get_values();
	for (ExpressionSyntax* value : values)
		value->accept(*this);

	ArrayTypeSymbol* arrayType = get_symbol_table().find_or_create_array_type(elementType, values.size());
	m_encoder.emit_new_array(arrayType);

	eval_pop(values.size());
	eval_push(sizeof(void*));
}

void ByteCodeEmitter::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// fixed-size array creation: new Type[size]
	if (!(node->get_array_size() == nullptr))
	{
		ArrayTypeSymbol* arrayType = lookup_symbol<ArrayTypeSymbol>(*nodeRef);
		if (arrayType == nullptr)
		{
			unsupported(*nodeRef, L"array creation without a bound type");
			return;
		}

		StructSymbol* elementType = substitute_type(arrayType->get_underlaying_type());
		if (elementType != arrayType->get_underlaying_type())
			arrayType = get_symbol_table().find_or_create_array_type(elementType, arrayType->get_length());

		node->get_array_size()->accept(*this);
		m_encoder.emit_new_array_dynamic(arrayType);
		eval_pop();
		eval_push(sizeof(void*));
		return;
	}

	emit_object_construction(node, std::nullopt);
}

void ByteCodeEmitter::emit_object_construction(const ObjectExpressionSyntax* node, std::optional<std::uint16_t> directSlot)
{
	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	SyntaxSymbol* boundSymbol = get_symbol_table().lookup_symbol(*nodeRef);
	ConstructorSymbol* constructor = nullptr;
	if (!(boundSymbol == nullptr) && boundSymbol->get_kind() == SyntaxKind::ConstructorDeclaration)
		constructor = static_cast<ConstructorSymbol*>(boundSymbol);

	StructSymbol* type = get_model().get_expression_type(*nodeRef);
	if ((type == nullptr) && !(constructor == nullptr))
		type = gmt::as<StructSymbol>(constructor->get_parent());

	type = substitute_type(type);
	if (type == nullptr)
	{
		unsupported(*nodeRef, L"object creation without a bound type");
		return;
	}

	MethodSymbol* constructorCall = nullptr;
	if (!(constructor == nullptr))
	{
		constructorCall = resolve_call_target(*nodeRef, constructor, type);
		if (constructorCall == nullptr)
			return;
	}

	PlacementVerdict verdict = site_verdict(*nodeRef);
	bool direct = directSlot.has_value() && verdict == PlacementVerdict::Stack;

	if (verdict == PlacementVerdict::Stack)
	{
		std::uint16_t slot = direct ? *directSlot : allocate_temporary(type, PlacementVerdict::Stack);

		emit_arguments_reversed(node->get_arguments());

		m_encoder.emit_load_slot_addr(slot);
		eval_push(sizeof(void*));

		if (!(constructorCall == nullptr))
		{
			m_encoder.emit_call_method_symbol(constructorCall);
			eval_pop(constructorCall->get_parameters().size() + 1);
		}

		if (!direct)
		{
			m_encoder.emit_load_slot_addr(slot);
			eval_push(sizeof(void*));
		}

		return;
	}

	std::uint16_t temp = allocate_temporary(type, PlacementVerdict::Heap);

	m_encoder.emit_alloc_heap(type);
	eval_push(sizeof(void*));
	m_encoder.emit_store_local(temp);
	eval_pop();

	emit_arguments_reversed(node->get_arguments());

	m_encoder.emit_load_local(temp);
	eval_push(sizeof(void*));

	if (!(constructorCall == nullptr))
	{
		m_encoder.emit_call_method_symbol(constructorCall);
		eval_pop(constructorCall->get_parameters().size() + 1);
	}

	m_encoder.emit_load_local(temp);
	eval_push(sizeof(void*));
}

void ByteCodeEmitter::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const LambdaExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	DelegateTypeSymbol* delegateType = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (delegateType == nullptr)
		return;

	MethodSymbol* anonymousMethod = nullptr;
	if (!(node->get_parameters() == nullptr))
		anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());

	if (anonymousMethod == nullptr)
		return;

	const SyntaxSymbol* parent = anonymousMethod->get_parent();
	StructSymbol* captureBox = (parent == nullptr) || !parent->is_type() ? nullptr : gmt::as<StructSymbol>(parent);
	if (captureBox == nullptr)
		return;

	// capture boxes are synthesized once for the open body: a lambda whose
	// signature or captures mention a type parameter would need its own box
	// per instantiation, which is out of scope for monomorphization
	if (!(m_currentInstantiation == nullptr))
	{
		bool typeParameterBound = substitute_type(anonymousMethod->get_return_type()) != anonymousMethod->get_return_type();

		if (!typeParameterBound)
		{
			for (ParameterSymbol* parameter : anonymousMethod->get_parameters())
			{
				if (substitute_type(parameter->get_type()) != parameter->get_type())
				{
					typeParameterBound = true;
					break;
				}
			}
		}

		if (!typeParameterBound)
		{
			for (FieldSymbol* field : captureBox->fields)
			{
				if (substitute_type(field->get_return_type()) != field->get_return_type())
				{
					typeParameterBound = true;
					break;
				}
			}
		}

		if (typeParameterBound)
		{
			unsupported(*nodeRef, L"lambdas using type-parameter typed signatures or captures");
			return;
		}
	}

	lambda_capture_checker checker(get_model(), get_diagnostics(), captureBox);
	if (!(node->get_body() == nullptr))
		node->get_body()->accept(checker);

	if (checker.get_captured())
	{
		unsupported(*nodeRef, L"capturing instance members");
		return;
	}

	m_pendingLambdaBodies.emplace_back(*nodeRef, anonymousMethod);

	// construct the capture box and store the captured values into its fields
	const gmt::vector<SyntaxSymbol*>* captures = m_pragmatic.get_lambda_captures(anonymousMethod);

	PlacementVerdict verdict = site_verdict(*nodeRef);
	std::uint16_t boxSlot = allocate_temporary(captureBox, verdict);
	bool stackBorn = verdict == PlacementVerdict::Stack;

	if (stackBorn)
	{
		m_encoder.emit_load_slot_addr(boxSlot);
		eval_push(sizeof(void*));
	}
	else
	{
		m_encoder.emit_alloc_heap(captureBox);
		eval_push(sizeof(void*));
		m_encoder.emit_store_local(boxSlot);
		eval_pop();
	}

	if (captures != nullptr)
	{
		for (std::size_t i = 0; i < captures->size() && i < captureBox->fields.size(); ++i)
		{
			FieldSymbol* field = captureBox->fields[i];
			SyntaxSymbol* captured = (*captures)[i];

			std::optional<std::uint16_t> capturedSlot = slot_of(captured);
			FieldSymbol* outerField = capturedSlot.has_value() ? nullptr : capture_field_of(captured);

			if (!capturedSlot.has_value() && (outerField == nullptr))
			{
				unsupported(*nodeRef, L"unresolvable lambda capture");
				return;
			}

			if (stackBorn)
			{
				m_encoder.emit_load_slot_addr(boxSlot);
				eval_push(sizeof(void*));
			}
			else
			{
				m_encoder.emit_load_local(boxSlot);
				eval_push(sizeof(void*));
			}

			// nested lambdas read transitively captured values through the
			// enclosing box's own capture fields
			if (capturedSlot.has_value())
			{
				m_encoder.emit_load_local(*capturedSlot);
				eval_push(payload_of(field->get_return_type()));
			}
			else
			{
				m_encoder.emit_load_local(0);
				eval_push(sizeof(void*));
				m_encoder.emit_load_field(field_slot_index(outerField));
				eval_push(payload_of(outerField->get_return_type()));
				eval_pop();
			}

			m_encoder.emit_store_field(field_slot_index(field));
			eval_pop(2);
		}
	}

	if (stackBorn)
	{
		m_encoder.emit_load_slot_addr(boxSlot);
		eval_push(sizeof(void*));
		return;
	}

	m_encoder.emit_load_local(boxSlot);
	eval_push(sizeof(void*));
}
