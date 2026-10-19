#include <shard/compilation/ByteCodeEmitter.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/TokenType.hpp>

#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>

using namespace shard;

ByteCodeEmitter::ByteCodeEmitter(VirtualProgramImage& image, SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
	, m_image(image)
	, m_pragmatic(pragmatic)
	, m_encoder(image)
{ }

const TypeShape* ByteCodeEmitter::shape_of(gmt::Ref<StructSymbol> type)
{
	if (type.is_null())
		return nullptr;

	return m_pragmatic.find_or_create_type_shape(get_model(), type);
}

std::size_t ByteCodeEmitter::payload_of(gmt::Ref<StructSymbol> type)
{
	const TypeShape* shape = shape_of(type);
	if (shape == nullptr)
		return sizeof(void*);

	if (shape->is_reference())
		return sizeof(void*);

	return shape->get_size();
}

void ByteCodeEmitter::eval_push(std::size_t payload)
{
	if (m_evalPoisoned)
		return;

	++m_evalDepth;
	m_maxEvalDepth = (std::max)(m_maxEvalDepth, static_cast<std::uint32_t>(m_evalDepth));
	m_maxEvalPayload = (std::max)(m_maxEvalPayload, payload);
}

void ByteCodeEmitter::eval_push_type(gmt::Ref<StructSymbol> type)
{
	eval_push(payload_of(type));
}

void ByteCodeEmitter::eval_pop(std::size_t count)
{
	if (m_evalPoisoned)
		return;

	m_evalDepth -= static_cast<std::int32_t>(count);
	if (m_evalDepth < 0)
	{
		m_evalPoisoned = true;
		m_evalDepth = 0;
	}
}

std::size_t ByteCodeEmitter::mark_position() const
{
	return m_image.get_text_section().size();
}

void ByteCodeEmitter::patch_jump(std::size_t at)
{
	std::size_t target = m_image.get_text_section().size();
	ByteCodeEncoder::paste_data(m_image.get_text_section(), at + sizeof(OpCode), &target, sizeof(std::size_t));
}

void ByteCodeEmitter::patch_target(std::size_t at, std::size_t target)
{
	ByteCodeEncoder::paste_data(m_image.get_text_section(), at + sizeof(OpCode), &target, sizeof(std::size_t));
}

std::optional<std::uint16_t> ByteCodeEmitter::slot_of(gmt::Ref<SyntaxSymbol> symbol) const
{
	if (symbol.is_null() || m_generatingFor.is_null())
		return std::nullopt;

	if (symbol->get_kind() == SyntaxKind::Parameter)
	{
		std::uint16_t index = m_generatingFor->get_linking() == LINK_INSTANCE ? 1 : 0;
		for (gmt::Ref<ParameterSymbol> parameter : m_generatingFor->get_parameters())
		{
			if (parameter == symbol)
				return index;

			++index;
		}

		return std::nullopt;
	}

	std::optional<std::uint16_t> slot = m_pragmatic.get_local_slot(m_generatingFor, symbol);
	if (slot.has_value())
		return slot;

	return std::nullopt;
}

std::optional<std::uint16_t> ByteCodeEmitter::scalar_field_slot(gmt::Ref<SyntaxSymbol> variable, gmt::Ref<FieldSymbol> field) const
{
	std::optional<std::uint16_t> base = m_pragmatic.get_local_slot(m_generatingFor, variable);
	if (!base.has_value())
		return std::nullopt;

	gmt::Ref<StructSymbol> ownerType = field->get_parent().as<StructSymbol>();
	if (ownerType.is_null())
		return std::nullopt;

	std::uint16_t ordinal = 0;
	for (gmt::Ref<FieldSymbol> candidate : ownerType->fields)
	{
		if (candidate.is_null() || candidate->get_return_type().is_null())
			continue;

		if (candidate->get_linking() == LINK_STATIC)
			continue;

		if (candidate == field)
			return static_cast<std::uint16_t>(*base + ordinal);

		++ordinal;
	}

	return std::nullopt;
}

std::uint16_t ByteCodeEmitter::allocate_temporary(gmt::Ref<StructSymbol> type, PlacementVerdict verdict)
{
	std::uint16_t variableIndex = m_layout->add_temporary(type, shape_of(type), verdict);
	return m_layout->get_argument_count() + variableIndex;
}

std::uint32_t ByteCodeEmitter::field_slot_index(gmt::Ref<FieldSymbol> field) const
{
	gmt::Ref<StructSymbol> ownerType = field->get_parent().as<StructSymbol>();
	if (ownerType.is_null())
		return 0;

	std::uint32_t ordinal = 0;
	for (gmt::Ref<FieldSymbol> candidate : ownerType->fields)
	{
		if (candidate.is_null() || candidate->get_return_type().is_null())
			continue;

		if (candidate->get_linking() == LINK_STATIC)
			continue;

		if (candidate == field)
			return ordinal;

		++ordinal;
	}

	return ordinal;
}

PlacementVerdict ByteCodeEmitter::local_verdict(gmt::Ref<SyntaxSymbol> symbol) const
{
	if (m_generatingFor.is_null())
		return PlacementVerdict::Heap;

	// the frame layout is the post-demotion truth for slot placement
	if (m_layout != nullptr)
	{
		std::optional<std::uint16_t> slot = m_pragmatic.get_local_slot(m_generatingFor, symbol);
		if (slot.has_value())
		{
			std::int32_t variableIndex = static_cast<std::int32_t>(*slot) - m_layout->get_argument_count();
			const std::vector<FrameSlotRecipe>& recipes = m_layout->get_variable_slots();

			if (variableIndex >= 0 && variableIndex < static_cast<std::int32_t>(recipes.size()))
				return recipes[static_cast<std::size_t>(variableIndex)].verdict;
		}
	}

	AllocationSiteVerdict verdict = m_pragmatic.get_local_verdict(m_generatingFor, symbol);
	if (verdict.verdict == PlacementVerdict::Promote || verdict.verdict == PlacementVerdict::StackReturn)
		return PlacementVerdict::Heap;

	return verdict.verdict;
}

PlacementVerdict ByteCodeEmitter::site_verdict(gmt::Ref<const ExpressionSyntax> site) const
{
	std::optional<AllocationSiteVerdict> verdict = m_pragmatic.get_allocation_site_verdict(site);
	if (!verdict.has_value())
		return PlacementVerdict::Heap;

	if (verdict->verdict == PlacementVerdict::Promote || verdict->verdict == PlacementVerdict::StackReturn)
		return PlacementVerdict::Heap;

	// scalar sites are identity-free and escape-free: plain stack placement
	if (verdict->verdict == PlacementVerdict::Scalar)
		return PlacementVerdict::Stack;

	return verdict->verdict;
}

void ByteCodeEmitter::begin_method(gmt::Ref<MethodSymbol> method)
{
	m_generatingFor = method;
	m_layout = m_pragmatic.find_frame_layout(method);
	m_methodStart = mark_position();

	m_loops.clear();
	m_clauseEndBacktracks.clear();
	m_deferScopes.clear();

	m_evalDepth = 0;
	m_maxEvalDepth = 0;
	m_maxEvalPayload = 0;
	m_evalPoisoned = false;
}

void ByteCodeEmitter::end_method()
{
	m_encoder.emit_return();

	std::size_t length = mark_position() - m_methodStart;
	m_image.register_method(m_generatingFor, m_methodStart, length);
	m_generatingFor->set_function_pointer(reinterpret_cast<void*>(m_methodStart));

	if (m_layout != nullptr)
		m_layout->publish_eval_layout(m_maxEvalDepth, m_maxEvalPayload, m_evalPoisoned);

	// nested method bodies (lambdas) are emitted after the outer method so
	// its text range stays contiguous; draining may enqueue more
	while (!m_pendingLambdaBodies.empty())
	{
		auto [node, method] = m_pendingLambdaBodies.front();
		m_pendingLambdaBodies.erase(m_pendingLambdaBodies.begin());

		begin_method(method);
		emit_body(node->get_body());
		end_method();
	}

	m_generatingFor = gmt::nullref;
	m_layout = nullptr;
}

void ByteCodeEmitter::emit_body(gmt::Ref<const BlockSyntax> body)
{
	if (body.is_null())
		return;

	if (body->get_kind() == SyntaxKind::ArrowClause)
	{
		const ArrowClauseSyntax* arrow = body.as<ArrowClauseSyntax>().as_ptr();
		if (!arrow->get_expression().is_null())
		{
			arrow->get_expression().as_ptr()->accept(*this);
			eval_pop();
		}

		return;
	}

	visit_statements_block(body.as<StatementsBlockSyntax>().as_ptr());
}

bool ByteCodeEmitter::method_supported(gmt::Ref<MethodSymbol> method, gmt::Ref<const SyntaxNode> blame)
{
	if (!method->get_type_parameters().empty())
	{
		unsupported(blame, L"generic methods");
		return false;
	}

	gmt::Ref<const SyntaxSymbol> parent = method->get_parent();
	if (!parent.is_null() && parent->is_type())
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
		if (!ownerType->type_parameters.empty())
		{
			unsupported(blame, L"methods of generic types");
			return false;
		}
	}

	if (method->get_is_async())
	{
		unsupported(blame, L"async methods");
		return false;
	}

	return true;
}

void ByteCodeEmitter::unsupported(gmt::Ref<const SyntaxNode> blame, const wchar_t* feature)
{
	TextLocation location;
	if (!blame.is_null())
		location = blame->get_location();

	get_diagnostics().report_error(location, gmt::concat(feature, L" is not yet supported by the code generator"));
}

void ByteCodeEmitter::emit(SyntaxTree& tree)
{
	visit_syntax_tree(tree);
	pick_entry_points();
}

bool ByteCodeEmitter::is_entry_point_candidate(gmt::Ref<MethodSymbol> method)
{
	if (method->get_name() != L"Main")
		return false;

	if (method->get_linking() != LINK_STATIC)
		return false;

	if (!method->get_parameters().empty())
		return false;

	return true;
}

void ByteCodeEmitter::pick_entry_points()
{
	if (m_entryPointCandidates.empty())
	{
		get_diagnostics().report_error(TextLocation(), L"program has no entry point: expected a static func Main()");
		return;
	}

	if (m_entryPointCandidates.size() > 1)
	{
		get_diagnostics().report_error(TextLocation(), L"program has multiple entry points named Main");
		return;
	}

	m_image.set_entry_point(m_entryPointCandidates.front());
}

void ByteCodeEmitter::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const FunctionDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	if (is_entry_point_candidate(symbol))
		m_entryPointCandidates.push_back(symbol);

	if (node->get_body().is_null())
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);
	emit_body(node->get_body());
	end_method();
}

void ByteCodeEmitter::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const OperatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null() || node->get_body().is_null())
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);
	emit_body(node->get_body());
	end_method();
}

void ByteCodeEmitter::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const ConstructorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null() || node->get_body().is_null())
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (!parent.is_null() && parent->is_type())
		emit_field_initializers(parent.as<StructSymbol>());

	emit_body(node->get_body());
	end_method();
}

void ByteCodeEmitter::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<gmt::Ref<const AccessorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<AccessorSymbol> symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);

	// accessors are semicolon-only: the body is synthesized over the backing field
	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	gmt::Ref<PropertySymbol> property = parent.is_null() ? gmt::nullref : parent.as<PropertySymbol>();
	gmt::Ref<FieldSymbol> backing = property.is_null() ? gmt::nullref : property->get_backing_field();

	if (!backing.is_null())
	{
		std::uint32_t fieldSlot = field_slot_index(backing);
		if (symbol->get_name() == L"get")
		{
			m_encoder.emit_load_arg(0);
			eval_push(sizeof(void*));
			m_encoder.emit_load_field(fieldSlot);
			eval_push_type(backing->get_return_type());
			eval_pop(2);
		}
		else
		{
			m_encoder.emit_load_arg(0);
			eval_push(sizeof(void*));
			m_encoder.emit_load_arg(1);
			eval_push_type(backing->get_return_type());
			m_encoder.emit_store_field(fieldSlot);
			eval_pop(2);
		}
	}

	end_method();
}

void ByteCodeEmitter::emit_field_initializers(gmt::Ref<StructSymbol> ownerType)
{
	for (gmt::Ref<FieldSymbol> field : ownerType->fields)
	{
		if (field.is_null() || field->get_linking() == LINK_STATIC)
			continue;

		gmt::Ref<const SyntaxNode> declaration = get_symbol_table().lookup_node(field);
		if (declaration.is_null() || declaration->get_kind() != SyntaxKind::FieldDeclaration)
			continue;

		const FieldDeclarationSyntax* fieldNode = declaration.as<FieldDeclarationSyntax>().as_ptr();
		if (fieldNode->get_expression().is_null())
			continue;

		m_encoder.emit_load_arg(0);
		eval_push(sizeof(void*));

		fieldNode->get_expression().as_ptr()->accept(*this);

		m_encoder.emit_store_field(field_slot_index(field));
		eval_pop(2);
	}
}
