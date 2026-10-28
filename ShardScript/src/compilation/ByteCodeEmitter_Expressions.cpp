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
	gmt::Ref<shard::DelegateTypeSymbol> unwrap_delegate_type(gmt::Ref<shard::StructSymbol> type)
	{
		if (type.is_null())
			return gmt::nullref;

		if (type->get_kind() == shard::SyntaxKind::DelegateType)
			return type.as<shard::DelegateTypeSymbol>();

		if (type->get_kind() == shard::SyntaxKind::GenericType)
		{
			gmt::Ref<shard::StructSymbol> underlaying = type.as<shard::GenericTypeSymbol>()->get_underlaying_type();
			if (!underlaying.is_null() && underlaying->get_kind() == shard::SyntaxKind::DelegateType)
				return underlaying.as<shard::DelegateTypeSymbol>();
		}

		return gmt::nullref;
	}

	class lambda_capture_checker final : public SemanticVisitor
	{
		gmt::Ref<StructSymbol> m_captureBox;
		bool m_captured = false;

		void check_bound(gmt::Ref<const SyntaxNode> node, gmt::Ref<const ExpressionSyntax> previous)
		{
			// variable and parameter captures resolve to capture box fields at
			// emission. Only implicit-'this' members are unsupported: anything
			// with an explicit receiver expression stays legal
			if (!previous.is_null())
				return;

			gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node);
			if (bound.is_null() || !bound->is_member())
				return;

			if (bound.as<MemberSymbol>()->get_linking() == LINK_STATIC)
				return;

			gmt::Ref<const SyntaxSymbol> parent = bound->get_parent();
			if (parent.is_null() || !parent->is_type())
				return;

			if (parent != m_captureBox)
				m_captured = true;
		}

	public:
		lambda_capture_checker(SemanticModel& model, DiagnosticsContext& diagnostics, gmt::Ref<StructSymbol> captureBox)
			: SemanticVisitor(model, diagnostics)
			, m_captureBox(captureBox)
		{ }

		bool get_captured() const
		{
			return m_captured;
		}

		void visit_member_access_expression(const MemberAccessExpressionSyntax* node) override
		{
			std::optional<gmt::Ref<const MemberAccessExpressionSyntax>> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				check_bound(*nodeRef, node->get_previous());

			SyntaxVisitor::visit_member_access_expression(node);
		}

		void visit_identifier_name_type(const IdentifierNameTypeSyntax* node) override
		{
			std::optional<gmt::Ref<const IdentifierNameTypeSyntax>> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				check_bound(*nodeRef, gmt::nullref);

			SyntaxVisitor::visit_identifier_name_type(node);
		}

		void visit_invokation_expression(const InvokationExpressionSyntax* node) override
		{
			std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);
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
			std::optional<gmt::Ref<const LiteralExpressionSyntax>> nodeRef = ref_from(node);
			gmt::Ref<LiteralSymbol> symbol = nodeRef.has_value() ? lookup_symbol<LiteralSymbol>(*nodeRef) : gmt::nullref;

			if (!symbol.is_null())
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
		node->get_left().as_ptr()->accept(*this);
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

		node->get_right().as_ptr()->accept(*this);
		patch_jump(endAt);
		return;
	}

	std::optional<gmt::Ref<const BinaryExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
	{
		gmt::Ref<OperatorSymbol> overload = lookup_symbol<OperatorSymbol>(*nodeRef);
		if (!overload.is_null())
		{
			gmt::Ref<StructSymbol> receiverType = get_model().get_expression_type(node->get_left());
			gmt::Ref<MethodSymbol> target = resolve_call_target(*nodeRef, overload, receiverType);
			if (target.is_null())
				return;

			node->get_right().as_ptr()->accept(*this);
			node->get_left().as_ptr()->accept(*this);

			m_encoder.emit_call_method_symbol(target);
			eval_pop(2);
			eval_push_type(target->get_return_type());
			return;
		}
	}

	node->get_left().as_ptr()->accept(*this);
	node->get_right().as_ptr()->accept(*this);

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
	std::optional<gmt::Ref<const UnaryExpressionSyntax>> nodeRef = ref_from(node);

	if (op == TokenType::TryKeyword)
	{
		node->get_operand().as_ptr()->accept(*this);
		return;
	}

	if (op == TokenType::NotOperator)
	{
		node->get_operand().as_ptr()->accept(*this);
		m_encoder.emit_logical_not();
		return;
	}

	if (op == TokenType::SubOperator)
	{
		node->get_operand().as_ptr()->accept(*this);
		m_encoder.emit_math_negative();
		return;
	}

	if (op == TokenType::IncrementOperator || op == TokenType::DecrementOperator)
	{
		bool increment = op == TokenType::IncrementOperator;
		const ExpressionSyntax* operand = node->get_operand().as_ptr();

		if (!is_identifier_target(operand))
		{
			unsupported(*nodeRef, L"increment of non-variable targets");
			return;
		}

		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node->get_operand());

		std::optional<std::uint16_t> slot;
		if (!bound.is_null() && (bound->get_kind() == SyntaxKind::VariableStatement || bound->get_kind() == SyntaxKind::Parameter))
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

	std::optional<gmt::Ref<const MemberAccessExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	emit_identifier_load(*nodeRef, node->get_previous());
}

void ByteCodeEmitter::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const IdentifierNameTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	emit_identifier_load(*nodeRef, gmt::nullref);
}

bool ByteCodeEmitter::is_identifier_target(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return false;

	SyntaxKind kind = expression->get_kind();
	return kind == SyntaxKind::MemberAccessExpression || kind == SyntaxKind::IdentifierNameType;
}

void ByteCodeEmitter::emit_identifier_load(gmt::Ref<const SyntaxNode> node, gmt::Ref<const ExpressionSyntax> previous)
{
	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node);
	if (bound.is_null())
	{
		if (!previous.is_null())
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
			gmt::Ref<FieldSymbol> captureField = capture_field_of(bound);
			if (!captureField.is_null())
			{
				emit_instance_field_load(captureField, gmt::nullref);
				return;
			}

			unsupported(node, L"lambda captures");
			return;
		}

		case SyntaxKind::FieldDeclaration:
		{
			gmt::Ref<FieldSymbol> field = bound.as<FieldSymbol>();
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
			gmt::Ref<PropertySymbol> property = bound.as<PropertySymbol>();
			gmt::Ref<MethodSymbol> getter = property->get_getter();

			if (getter.is_null())
			{
				unsupported(node, L"property without a getter");
				return;
			}

			gmt::Ref<StructSymbol> receiverType = previous.is_null() ? gmt::nullref : get_model().get_expression_type(previous);
			getter = resolve_call_target(node, getter, receiverType);
			if (getter.is_null())
				return;

			if (!previous.is_null())
				previous.as_ptr()->accept(*this);
			else
			{
				m_encoder.emit_load_local(0);
				eval_push(sizeof(void*));
			}

			m_encoder.emit_call_method_symbol(getter);
			eval_pop();

			// the binder's expression type carries the substituted value type;
			// the open declared type can still be a type parameter
			gmt::Ref<StructSymbol> valueType = get_model().get_expression_type(node.as<const ExpressionSyntax>());
			eval_push_type(valueType.is_null() ? getter->get_return_type() : valueType);
			return;
		}

		default:
		{
			unsupported(node, L"this member access shape");
			return;
		}
	}
}

void ByteCodeEmitter::emit_instance_field_load(gmt::Ref<FieldSymbol> field, gmt::Ref<const ExpressionSyntax> previous)
{
	// scalar locals are dematerialized: the field is a frame slot of its own
	if (!previous.is_null() && is_identifier_target(previous.as_ptr()))
	{
		gmt::Ref<SyntaxSymbol> previousBound = get_symbol_table().lookup_symbol(previous);
		if (!previousBound.is_null() && local_verdict(previousBound) == PlacementVerdict::Scalar)
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

	if (!previous.is_null())
		previous.as_ptr()->accept(*this);
	else
	{
		m_encoder.emit_load_local(0);
		eval_push(sizeof(void*));
	}

	m_encoder.emit_load_field(field_slot_index(field));
	eval_push(payload_of(field->get_return_type()));
	eval_pop();
}

gmt::Ref<FieldSymbol> ByteCodeEmitter::capture_field_of(gmt::Ref<SyntaxSymbol> bound) const
{
	if (m_generatingFor.is_null())
		return gmt::nullref;

	const gmt::vector<gmt::Ref<SyntaxSymbol>>* captures = m_pragmatic.get_lambda_captures(m_generatingFor);
	if (captures == nullptr)
		return gmt::nullref;

	gmt::Ref<const SyntaxSymbol> parent = m_generatingFor->get_parent();
	if (parent.is_null() || !parent->is_type())
		return gmt::nullref;

	gmt::Ref<StructSymbol> captureBox = parent.as<StructSymbol>();
	for (std::size_t i = 0; i < captures->size() && i < captureBox->fields.size(); ++i)
	{
		if ((*captures)[i] == bound)
			return captureBox->fields[i];
	}

	return gmt::nullref;
}

void ByteCodeEmitter::emit_arguments_reversed(const ArgumentsListSyntax* arguments)
{
	if (arguments == nullptr)
		return;

	std::span<const gmt::Ref<ArgumentSyntax>> list = arguments->get_arguments().as_span();
	for (std::size_t i = list.size(); i-- > 0;)
	{
		const ArgumentSyntax* argument = list[i].as_ptr();
		if (argument->get_expression().is_null())
			continue;

		argument->get_expression().as_ptr()->accept(*this);
	}
}

bool ByteCodeEmitter::is_extension_call(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> receiver, const ArgumentsListSyntax* arguments)
{
	if (receiver.is_null() || method->get_linking() != LINK_STATIC)
		return false;

	// the binder matches scope functions against the receiver prepended to
	// the argument list, so a bound static callee with one more parameter
	// than explicit arguments consumes the receiver as its first parameter
	gmt::Ref<StructSymbol> receiverType = get_model().get_expression_type(receiver);
	if (receiverType.is_null())
		return false;

	std::size_t explicitArguments = arguments == nullptr ? 0 : arguments->get_arguments().as_span().size();
	return method->get_parameters().size() == explicitArguments + 1;
}

void ByteCodeEmitter::emit_method_call(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> receiver, const ArgumentsListSyntax* arguments, bool receiverFirst)
{
	bool isStatic = method->get_linking() == LINK_STATIC;
	bool extension = is_extension_call(method, receiver, arguments);

	gmt::Ref<const SyntaxSymbol> parent = method->get_parent();
	bool interfaceDispatch = !parent.is_null() && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

	emit_arguments_reversed(arguments);

	std::size_t receiverEval = 0;
	if (!isStatic || extension)
	{
		if (!receiver.is_null())
			receiver.as_ptr()->accept(*this);
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

	if (!method->get_return_type().is_null() && method->get_return_type() != get_symbol_table().get_void_primitive())
		eval_push_type(method->get_return_type());
}

void ByteCodeEmitter::emit_method_call_into(gmt::Ref<MethodSymbol> method, gmt::Ref<const ExpressionSyntax> receiver, const ArgumentsListSyntax* arguments, std::uint16_t slot)
{
	bool isStatic = method->get_linking() == LINK_STATIC;
	bool extension = is_extension_call(method, receiver, arguments);

	emit_arguments_reversed(arguments);

	std::size_t receiverEval = 0;
	if (!isStatic || extension)
	{
		if (!receiver.is_null())
			receiver.as_ptr()->accept(*this);
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

gmt::Ref<MethodSymbol> ByteCodeEmitter::into_call_method(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null() || expression->get_kind() != SyntaxKind::InvokationExpression)
		return gmt::nullref;

	const InvokationExpressionSyntax* invokation = static_cast<const InvokationExpressionSyntax*>(expression.as_ptr());
	std::optional<gmt::Ref<const InvokationExpressionSyntax>> invokationRef = ref_from(invokation);
	if (!invokationRef.has_value())
		return gmt::nullref;

	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*invokationRef);
	if (bound.is_null() || !bound->is_method())
		return gmt::nullref;

	gmt::Ref<MethodSymbol> method = bound.as<MethodSymbol>();

	gmt::Ref<StructSymbol> receiverType = gmt::nullref;
	if (!invokation->get_previous().is_null())
		receiverType = get_model().get_expression_type(invokation->get_previous());

	method = resolve_call_target(*invokationRef, method, receiverType);
	if (method.is_null())
		return gmt::nullref;

	gmt::Ref<const SyntaxSymbol> parent = method->get_parent();
	if (!parent.is_null() && parent->get_kind() == SyntaxKind::InterfaceDeclaration)
		return gmt::nullref;

	gmt::Ref<StructSymbol> returnType = method->get_return_type();
	if (returnType.is_null())
		return gmt::nullref;

	SyntaxKind returnKind = returnType->get_kind();
	if (returnKind != SyntaxKind::StructDeclaration && returnKind != SyntaxKind::GenericType)
		return gmt::nullref;

	return method;
}

void ByteCodeEmitter::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// direct callee expression form: (callee)(arguments)
	if (node->get_identifier().get_type() == TokenType::Unknown && !node->get_previous().is_null())
	{
		gmt::Ref<StructSymbol> calleeType = get_model().get_expression_type(node->get_previous());
		gmt::Ref<DelegateTypeSymbol> delegateType = unwrap_delegate_type(calleeType);

		if (delegateType.is_null())
		{
			unsupported(*nodeRef, L"invokation of a non-delegate expression");
			return;
		}

		gmt::Ref<GenericTypeSymbol> instantiation = calleeType->get_kind() == SyntaxKind::GenericType
			? calleeType.as<GenericTypeSymbol>()
			: gmt::nullref;

		emit_arguments_reversed(node->get_arguments().as_ptr());
		node->get_previous().as_ptr()->accept(*this);

		m_encoder.emit_call_delegate(delegateType);

		std::size_t argumentCount = delegateType->get_parameters().size() + 1;
		eval_pop(argumentCount);
		eval_push_type(get_model().substitute_type(delegateType->get_return_type(), instantiation));
		return;
	}

	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (bound.is_null())
		return;

	if (bound->is_method())
	{
		gmt::Ref<MethodSymbol> method = bound.as<MethodSymbol>();

		gmt::Ref<StructSymbol> receiverType = gmt::nullref;
		if (!node->get_previous().is_null())
			receiverType = get_model().get_expression_type(node->get_previous());

		method = resolve_call_target(*nodeRef, method, receiverType);
		if (method.is_null())
			return;

		emit_method_call(method, node->get_previous(), node->get_arguments().as_ptr(), false);
		return;
	}

	// delegate invocation through a variable, parameter or field
	SyntaxKind kind = bound->get_kind();
	gmt::Ref<StructSymbol> holderType = gmt::nullref;

	if (kind == SyntaxKind::VariableStatement)
		holderType = bound.as<VariableSymbol>()->get_type();
	else if (kind == SyntaxKind::Parameter)
		holderType = bound.as<ParameterSymbol>()->get_type();
	else if (kind == SyntaxKind::FieldDeclaration)
		holderType = bound.as<FieldSymbol>()->get_return_type();

	gmt::Ref<DelegateTypeSymbol> delegateType = unwrap_delegate_type(holderType);

	if (delegateType.is_null())
	{
		unsupported(*nodeRef, L"this invokation shape");
		return;
	}

	// the callee value loads like a member access over the same name, on top
	emit_arguments_reversed(node->get_arguments().as_ptr());

	if (kind == SyntaxKind::FieldDeclaration)
	{
		gmt::Ref<FieldSymbol> field = bound.as<FieldSymbol>();
		if (field->get_linking() == LINK_STATIC)
		{
			m_encoder.emit_load_static_field(field);
			eval_push(sizeof(void*));
		}
		else
		{
			if (!node->get_previous().is_null())
				node->get_previous().as_ptr()->accept(*this);
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

	gmt::Ref<GenericTypeSymbol> instantiation = !holderType.is_null() && holderType->get_kind() == SyntaxKind::GenericType
		? holderType.as<GenericTypeSymbol>()
		: gmt::nullref;

	std::size_t argumentCount = delegateType->get_parameters().size() + 1;
	eval_pop(argumentCount);
	eval_push_type(get_model().substitute_type(delegateType->get_return_type(), instantiation));
}

void ByteCodeEmitter::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const IndexatorExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<IndexatorSymbol> indexator = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (!indexator.is_null())
	{
		gmt::Ref<MethodSymbol> getter = indexator->get_getter();
		if (getter.is_null())
		{
			unsupported(*nodeRef, L"indexator without a getter");
			return;
		}

		gmt::Ref<StructSymbol> receiverType = node->get_previous().is_null() ? gmt::nullref : get_model().get_expression_type(node->get_previous());
		getter = resolve_call_target(*nodeRef, getter, receiverType);
		if (getter.is_null())
			return;

		emit_arguments_reversed(node->get_arguments().as_ptr());
		if (!node->get_previous().is_null())
			node->get_previous().as_ptr()->accept(*this);
		else
		{
			m_encoder.emit_load_local(0);
			eval_push(sizeof(void*));
		}

		m_encoder.emit_call_method_symbol(getter);

		std::size_t argumentCount = indexator->get_parameters().size() + 1;
		eval_pop(argumentCount);

		gmt::Ref<StructSymbol> valueType = get_model().get_expression_type(*nodeRef);
		eval_push_type(valueType.is_null() ? getter->get_return_type() : valueType);
		return;
	}

	// array element access
	node->get_previous().as_ptr()->accept(*this);

	gmt::Span<const gmt::Ref<ArgumentSyntax>> arguments = node->get_arguments()->get_arguments();
	if (!arguments.as_span().empty())
		arguments.as_span().front()->get_expression().as_ptr()->accept(*this);

	m_encoder.emit_load_array_element();
	eval_pop();
}

void ByteCodeEmitter::visit_is_expression(const IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	gmt::Ref<StructSymbol> targetType;
	if (!node->get_target_type().is_null())
	{
		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node->get_target_type());
		if (!bound.is_null() && bound->is_type())
			targetType = bound.as<StructSymbol>();
	}

	targetType = substitute_type(targetType);

	node->get_expression().as_ptr()->accept(*this);
	m_encoder.emit_is_instance(targetType);
	eval_push(sizeof(bool));
	eval_pop(2);
}

void ByteCodeEmitter::visit_cast_expression(const CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	gmt::Ref<StructSymbol> targetType;
	if (!node->get_target_type().is_null())
	{
		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node->get_target_type());
		if (!bound.is_null() && bound->is_type())
			targetType = bound.as<StructSymbol>();
	}

	targetType = substitute_type(targetType);

	node->get_expression().as_ptr()->accept(*this);

	if (!targetType.is_null() && get_model().is_primitive_type(targetType))
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

	node->get_condition().as_ptr()->accept(*this);
	eval_pop();

	std::size_t elseAt = mark_position();
	m_encoder.emit_jump_false(0);

	std::int32_t armDepth = m_evalDepth;
	node->get_then_expression().as_ptr()->accept(*this);

	std::size_t endAt = mark_position();
	m_encoder.emit_jump(0);

	patch_jump(elseAt);
	m_evalDepth = armDepth;

	node->get_else_expression().as_ptr()->accept(*this);
	patch_jump(endAt);
}

void ByteCodeEmitter::visit_await_expression(const AwaitExpressionSyntax* node)
{
	std::optional<gmt::Ref<const AwaitExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		unsupported(*nodeRef, L"await expressions");
}

void ByteCodeEmitter::visit_range_expression(const RangeExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	node->get_left().as_ptr()->accept(*this);
	node->get_right().as_ptr()->accept(*this);

	bool inclusive = node->get_operator_token().get_type() == TokenType::RangeInclusiveOperator;
	m_encoder.emit_load_const_bool(inclusive);
	eval_push(sizeof(bool));

	gmt::Ref<StructSymbol> rangeType = get_symbol_table().get_standard_type(StandardTypes::RangeEnumerable).as<StructSymbol>();
	m_encoder.emit_create_range(rangeType);
	eval_pop(3);
	eval_push(sizeof(void*));
}

void ByteCodeEmitter::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const CollectionExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<ArrayTypeSymbol> boundArray = nodeRef.has_value() ? lookup_symbol<ArrayTypeSymbol>(*nodeRef) : gmt::nullref;

	gmt::Ref<StructSymbol> elementType = boundArray.is_null() ? gmt::nullref : boundArray->get_underlaying_type();
	elementType = substitute_type(elementType);
	if (elementType.is_null())
	{
		unsupported(*nodeRef, L"collection expression without a bound element type");
		return;
	}

	gmt::Span<const gmt::Ref<ExpressionSyntax>> values = node->get_values();
	for (gmt::Ref<ExpressionSyntax> value : values.as_span())
		value.as_ptr()->accept(*this);

	gmt::Ref<ArrayTypeSymbol> arrayType = get_symbol_table().find_or_create_array_type(elementType, values.as_span().size());
	m_encoder.emit_new_array(arrayType);

	eval_pop(values.as_span().size());
	eval_push(sizeof(void*));
}

void ByteCodeEmitter::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// fixed-size array creation: new Type[size]
	if (!node->get_array_size().is_null())
	{
		gmt::Ref<ArrayTypeSymbol> arrayType = lookup_symbol<ArrayTypeSymbol>(*nodeRef);
		if (arrayType.is_null())
		{
			unsupported(*nodeRef, L"array creation without a bound type");
			return;
		}

		gmt::Ref<StructSymbol> elementType = substitute_type(arrayType->get_underlaying_type());
		if (elementType != arrayType->get_underlaying_type())
			arrayType = get_symbol_table().find_or_create_array_type(elementType, arrayType->get_length());

		node->get_array_size().as_ptr()->accept(*this);
		m_encoder.emit_new_array_dynamic(arrayType);
		eval_pop();
		eval_push(sizeof(void*));
		return;
	}

	emit_object_construction(node, std::nullopt);
}

void ByteCodeEmitter::emit_object_construction(const ObjectExpressionSyntax* node, std::optional<std::uint16_t> directSlot)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> boundSymbol = get_symbol_table().lookup_symbol(*nodeRef);
	gmt::Ref<ConstructorSymbol> constructor = gmt::nullref;
	if (!boundSymbol.is_null() && boundSymbol->get_kind() == SyntaxKind::ConstructorDeclaration)
		constructor = boundSymbol.as<ConstructorSymbol>();

	gmt::Ref<StructSymbol> type = get_model().get_expression_type(*nodeRef);
	if (type.is_null() && !constructor.is_null())
		type = constructor->get_parent().as<StructSymbol>();

	type = substitute_type(type);
	if (type.is_null())
	{
		unsupported(*nodeRef, L"object creation without a bound type");
		return;
	}

	gmt::Ref<MethodSymbol> constructorCall = gmt::nullref;
	if (!constructor.is_null())
	{
		constructorCall = resolve_call_target(*nodeRef, constructor, type);
		if (constructorCall.is_null())
			return;
	}

	PlacementVerdict verdict = site_verdict(*nodeRef);
	bool direct = directSlot.has_value() && verdict == PlacementVerdict::Stack;

	if (verdict == PlacementVerdict::Stack)
	{
		std::uint16_t slot = direct ? *directSlot : allocate_temporary(type, PlacementVerdict::Stack);

		emit_arguments_reversed(node->get_arguments().as_ptr());

		m_encoder.emit_load_slot_addr(slot);
		eval_push(sizeof(void*));

		if (!constructorCall.is_null())
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

	emit_arguments_reversed(node->get_arguments().as_ptr());

	m_encoder.emit_load_local(temp);
	eval_push(sizeof(void*));

	if (!constructorCall.is_null())
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

	std::optional<gmt::Ref<const LambdaExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<DelegateTypeSymbol> delegateType = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (delegateType.is_null())
		return;

	gmt::Ref<MethodSymbol> anonymousMethod = gmt::nullref;
	if (!node->get_parameters().is_null())
		anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());

	if (anonymousMethod.is_null())
		return;

	gmt::Ref<const SyntaxSymbol> parent = anonymousMethod->get_parent();
	gmt::Ref<StructSymbol> captureBox = parent.is_null() || !parent->is_type() ? gmt::nullref : parent.as<StructSymbol>();
	if (captureBox.is_null())
		return;

	// capture boxes are synthesized once for the open body: a lambda whose
	// signature or captures mention a type parameter would need its own box
	// per instantiation, which is out of scope for monomorphization
	if (!m_currentInstantiation.is_null())
	{
		bool typeParameterBound = substitute_type(anonymousMethod->get_return_type()) != anonymousMethod->get_return_type();

		if (!typeParameterBound)
		{
			for (gmt::Ref<ParameterSymbol> parameter : anonymousMethod->get_parameters())
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
			for (gmt::Ref<FieldSymbol> field : captureBox->fields)
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
	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(checker);

	if (checker.get_captured())
	{
		unsupported(*nodeRef, L"capturing instance members");
		return;
	}

	m_pendingLambdaBodies.emplace_back(*nodeRef, anonymousMethod);

	// construct the capture box and store the captured values into its fields
	const gmt::vector<gmt::Ref<SyntaxSymbol>>* captures = m_pragmatic.get_lambda_captures(anonymousMethod);

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
			gmt::Ref<FieldSymbol> field = captureBox->fields[i];
			gmt::Ref<SyntaxSymbol> captured = (*captures)[i];

			std::optional<std::uint16_t> capturedSlot = slot_of(captured);
			gmt::Ref<FieldSymbol> outerField = capturedSlot.has_value() ? gmt::nullref : capture_field_of(captured);

			if (!capturedSlot.has_value() && outerField.is_null())
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
