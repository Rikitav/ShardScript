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
#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericMethodSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

using namespace shard;

ByteCodeEmitter::ByteCodeEmitter(VirtualProgramImage& image, SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
	, m_image(image)
	, m_pragmatic(pragmatic)
	, m_encoder(image)
{ }

const TypeShape* ByteCodeEmitter::shape_of(StructSymbol* type)
{
	if (type == nullptr)
		return nullptr;

	type = substitute_type(type);
	return m_pragmatic.find_or_create_type_shape(get_model(), type);
}

std::size_t ByteCodeEmitter::payload_of(StructSymbol* type)
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

void ByteCodeEmitter::eval_push_type(StructSymbol* type)
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

std::optional<std::uint16_t> ByteCodeEmitter::slot_of(SyntaxSymbol* symbol) const
{
	if ((symbol == nullptr) || (m_generatingFor == nullptr))
		return std::nullopt;

	if (symbol->get_kind() == SyntaxKind::Parameter)
	{
		std::uint16_t index = m_generatingFor->get_linking() == LINK_INSTANCE ? 1 : 0;
		for (ParameterSymbol* parameter : m_generatingFor->get_parameters())
		{
			if (parameter == symbol)
				return index;

			++index;
		}

		// the implicit receiver parameter is scope-only and never appears in
		// the method's parameter list; by convention it occupies slot 0
		bool implicitThis = m_generatingFor->get_linking() == LINK_INSTANCE;
		implicitThis = implicitThis && symbol->get_name() == L"this";
		implicitThis = implicitThis && symbol->get_parent() == m_generatingFor;
		if (implicitThis)
			return static_cast<std::uint16_t>(0);

		return std::nullopt;
	}

	std::optional<std::uint16_t> slot = m_pragmatic.get_local_slot(m_generatingFor, symbol);
	if (slot.has_value())
		return slot;

	return std::nullopt;
}

std::optional<std::uint16_t> ByteCodeEmitter::scalar_field_slot(SyntaxSymbol* variable, FieldSymbol* field) const
{
	std::optional<std::uint16_t> base = m_pragmatic.get_local_slot(m_generatingFor, variable);
	if (!base.has_value())
		return std::nullopt;

	StructSymbol* ownerType = gmt::as<StructSymbol>(field->get_parent());
	if (ownerType == nullptr)
		return std::nullopt;

	std::uint16_t ordinal = 0;
	for (FieldSymbol* candidate : ownerType->fields)
	{
		if ((candidate == nullptr) || (candidate->get_return_type() == nullptr))
			continue;

		if (candidate->get_linking() == LINK_STATIC)
			continue;

		if (candidate == field)
			return static_cast<std::uint16_t>(*base + ordinal);

		++ordinal;
	}

	return std::nullopt;
}

std::uint16_t ByteCodeEmitter::allocate_temporary(StructSymbol* type, PlacementVerdict verdict)
{
	type = substitute_type(type);
	std::uint16_t variableIndex = m_layout->add_temporary(type, shape_of(type), verdict);
	return m_layout->get_argument_count() + variableIndex;
}

std::uint32_t ByteCodeEmitter::field_slot_index(FieldSymbol* field) const
{
	StructSymbol* ownerType = gmt::as<StructSymbol>(field->get_parent());
	if (ownerType == nullptr)
		return 0;

	std::uint32_t ordinal = 0;
	for (FieldSymbol* candidate : ownerType->fields)
	{
		if ((candidate == nullptr) || (candidate->get_return_type() == nullptr))
			continue;

		if (candidate->get_linking() == LINK_STATIC)
			continue;

		if (candidate == field)
			return ordinal;

		++ordinal;
	}

	return ordinal;
}

PlacementVerdict ByteCodeEmitter::local_verdict(SyntaxSymbol* symbol) const
{
	if (m_generatingFor == nullptr)
		return PlacementVerdict::Heap;

	// the frame layout is the post-demotion truth for slot placement
	if (m_layout != nullptr)
	{
		std::optional<std::uint16_t> slot = m_pragmatic.get_local_slot(m_generatingFor, symbol);
		if (slot.has_value())
		{
			std::int32_t variableIndex = static_cast<std::int32_t>(*slot) - m_layout->get_argument_count();
			const gmt::vector<FrameSlotRecipe>& recipes = m_layout->get_variable_slots();

			if (variableIndex >= 0 && variableIndex < static_cast<std::int32_t>(recipes.size()))
				return recipes[static_cast<std::size_t>(variableIndex)].verdict;
		}
	}

	AllocationSiteVerdict verdict = m_pragmatic.get_local_verdict(m_generatingFor, symbol);
	if (verdict.verdict == PlacementVerdict::Promote || verdict.verdict == PlacementVerdict::StackReturn)
		return PlacementVerdict::Heap;

	return verdict.verdict;
}

PlacementVerdict ByteCodeEmitter::site_verdict(const ExpressionSyntax* site) const
{
	std::optional<AllocationSiteVerdict> verdict = m_pragmatic.get_allocation_site_verdict(site);
	if (!verdict.has_value())
		return PlacementVerdict::Heap;

	if (verdict->verdict == PlacementVerdict::Promote)
		return PlacementVerdict::Heap;

	// return-chained sites construct into the caller-provided return storage
	if (verdict->verdict == PlacementVerdict::StackReturn)
		return PlacementVerdict::Stack;

	// scalar sites are identity-free and escape-free: plain stack placement
	if (verdict->verdict == PlacementVerdict::Scalar)
		return PlacementVerdict::Stack;

	return verdict->verdict;
}

void ByteCodeEmitter::begin_method(MethodSymbol* method)
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
	m_generatingFor->set_calling_convention(CallingConvention::Managed);
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

	m_generatingFor = nullptr;
	m_layout = nullptr;
}

void ByteCodeEmitter::emit_body(const BlockSyntax* body)
{
	if (body == nullptr)
		return;

	if (body->get_kind() == SyntaxKind::ArrowClause)
	{
		const ArrowClauseSyntax* arrow = gmt::as<ArrowClauseSyntax>(body);
		if (!(arrow->get_expression() == nullptr))
		{
			arrow->get_expression()->accept(*this);

			// an expression body yields its value; end_method's RETURN pops it
			StructSymbol* returnType = (m_generatingFor == nullptr) ? nullptr : m_generatingFor->get_return_type();
			bool dropsResult = (returnType == nullptr) || returnType == get_symbol_table().get_void_primitive();
			if (dropsResult)
				eval_pop();
		}

		return;
	}

	visit_statements_block(gmt::as<StatementsBlockSyntax>(body));
}

bool ByteCodeEmitter::method_supported([[maybe_unused]] MethodSymbol* method, [[maybe_unused]] const SyntaxNode* blame)
{
	// async methods run through the ordinary bytecode; their invocation
	// boundary creates the Task wrapper instead of rewriting the body
	return true;
}

StructSymbol* ByteCodeEmitter::substitute_type(StructSymbol* type)
{
	if ((type == nullptr) || (m_currentInstantiation == nullptr))
		return type;

	return get_model().substitute_type(type, m_currentInstantiation->get_type_arguments());
}

StructSymbol* ByteCodeEmitter::owner_type_of(MethodSymbol* method)
{
	// accessors are parented to their property, so walk to the enclosing type
	const SyntaxSymbol* current = method->get_parent();
	while (!(current == nullptr))
	{
		if (current->is_type())
			return gmt::as<StructSymbol>(current);

		current = current->get_parent();
	}

	return nullptr;
}

bool ByteCodeEmitter::is_generic_definition(MethodSymbol* method)
{
	if (method == nullptr)
		return false;

	if (!method->get_type_parameters().empty())
		return true;

	StructSymbol* ownerType = owner_type_of(method);
	return !(ownerType == nullptr) && !ownerType->type_parameters.empty();
}

MethodSymbol* ByteCodeEmitter::resolve_call_target(const SyntaxNode* node, MethodSymbol* method, StructSymbol* receiverType)
{
	if ((method == nullptr) || method->get_is_generic_instantiation())
		return method;

	if (method->get_calling_convention() == CallingConvention::ShardDecl)
		return method;

	StructSymbol* ownerType = owner_type_of(method);
	const bool ownerGeneric = !(ownerType == nullptr) && !ownerType->type_parameters.empty();
	const bool methodGeneric = !method->get_type_parameters().empty();

	if (!ownerGeneric && !methodGeneric)
		return method;

	bool interfaceOwner = !(ownerType == nullptr) && ownerType->get_kind() == SyntaxKind::InterfaceDeclaration;
	if (interfaceOwner || method->get_is_virtual())
	{
		unsupported(node, L"virtual generic method invokations");
		return nullptr;
	}

	// combined index space: owner type arguments first, then the method's own;
	// matches TypeParameterSymbol::get_type_argument_index
	gmt::inline_vector<StructSymbol*, 8> combinedStorage;
	std::pmr::vector<StructSymbol*>& combined = combinedStorage.get_vector();

	std::span<StructSymbol* const> boundArguments = get_symbol_table().lookup_type_arguments(node);
	if (!boundArguments.empty())
	{
		combined.assign(boundArguments.begin(), boundArguments.end());
	}
	else if (methodGeneric)
	{
		// no inference site reached the emitter; the binder already reported it
		return nullptr;
	}
	else if (!(receiverType == nullptr) && receiverType->get_kind() == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* constructed = static_cast<GenericTypeSymbol*>(receiverType);
		for (TypeParameterSymbol* parameter : ownerType->type_parameters)
			combined.push_back(constructed->substitute_type_parameter(parameter));
	}
	else
	{
		// this-routed call inside a generic body: identity arguments that the
		// enclosing instantiation context resolves below
		for (TypeParameterSymbol* parameter : ownerType->type_parameters)
			combined.push_back(parameter);
	}

	for (StructSymbol*& argument : combined)
	{
		argument = substitute_type(argument);
		if ((argument == nullptr) || argument->get_kind() == SyntaxKind::TypeParameter)
			return nullptr;
	}

	std::span<StructSymbol* const> argumentsSpan(combined.data(), combined.size());
	StructSymbol* returnType = get_model().substitute_type(method->get_return_type(), argumentsSpan);

	GenericMethodSymbol* instantiation = get_symbol_table().find_or_create_generic_method(method, argumentsSpan, returnType);
	if (instantiation == nullptr)
		return nullptr;

	m_pragmatic.get_or_create_frame_layout(get_model(), instantiation);
	if (!enqueue_instantiation(instantiation))
		return nullptr;

	return instantiation;
}

bool ByteCodeEmitter::enqueue_instantiation(GenericMethodSymbol* instantiation)
{
	if (!m_enqueuedInstantiations.insert(reinterpret_cast<std::size_t>(instantiation)).second)
		return true;

	constexpr std::size_t InstantiationLimit = 256;
	if (m_instantiationsCreated >= InstantiationLimit)
	{
		// recursive generic expansions have no finite monomorphization; one
		// diagnostic is enough, every affected call target resolves to null
		if (!m_instantiationLimitReported)
		{
			m_instantiationLimitReported = true;
			get_diagnostics().report_error(TextLocation(), L"generic instantiation limit exceeded");
		}

		return false;
	}

	++m_instantiationsCreated;
	m_pendingInstantiations.push_back(instantiation);
	return true;
}

void ByteCodeEmitter::emit_pending_instantiations()
{
	// emitting a body may resolve further generic call targets
	for (std::size_t i = 0; i < m_pendingInstantiations.size(); ++i)
		emit_instantiation_body(m_pendingInstantiations[i]);

	m_pendingInstantiations.clear();
	m_enqueuedInstantiations.clear();
}

void ByteCodeEmitter::emit_instantiation_body(GenericMethodSymbol* instantiation)
{
	MethodSymbol* open = instantiation->get_underlaying_method();
	const SyntaxNode* declaration = get_symbol_table().lookup_node(open);

	// the symbol-to-node map keeps the first binding: declared methods map to
	// their declaration, synthesized constructors to an arbitrary construction
	// site
	const SyntaxKind declarationKind = (declaration == nullptr) ? SyntaxKind::Unknown : declaration->get_kind();

	// slot lookups stay keyed by the open method; only the layout and the
	// substitution context come from the instantiation
	begin_method(open);
	m_layout = &m_pragmatic.get_or_create_frame_layout(get_model(), instantiation);
	m_currentInstantiation = instantiation;

	switch (declarationKind)
	{
		case SyntaxKind::FunctionDeclaration:
		{
			const FunctionDeclarationSyntax* node = gmt::as<FunctionDeclarationSyntax>(declaration);
			emit_body(node->get_body());
			break;
		}

		case SyntaxKind::OperatorDeclaration:
		{
			const OperatorDeclarationSyntax* node = gmt::as<OperatorDeclarationSyntax>(declaration);
			emit_body(node->get_body());
			break;
		}

		case SyntaxKind::ConstructorDeclaration:
		{
			const ConstructorDeclarationSyntax* node = gmt::as<ConstructorDeclarationSyntax>(declaration);
			const SyntaxSymbol* parent = open->get_parent();
			if (!(parent == nullptr) && parent->is_type())
				emit_field_initializers(gmt::as<StructSymbol>(parent));

			emit_body(node->get_body());
			break;
		}

		case SyntaxKind::AccessorDeclaration:
		{
			AccessorSymbol* accessor = static_cast<AccessorSymbol*>(open);
			if (!(accessor == nullptr))
				emit_accessor_body(accessor);

			break;
		}

		default:
		{
			// synthesized implicit constructor: the body is field initializers only
			if (open->get_kind() == SyntaxKind::ConstructorDeclaration)
			{
				const SyntaxSymbol* parent = open->get_parent();
				if (!(parent == nullptr) && parent->is_type())
					emit_field_initializers(gmt::as<StructSymbol>(parent));
			}

			break;
		}
	}

	m_generatingFor = instantiation;
	end_method();
	m_currentInstantiation = nullptr;
}

void ByteCodeEmitter::unsupported(const SyntaxNode* blame, const wchar_t* feature)
{
	TextLocation location;
	if (!(blame == nullptr))
		location = blame->get_location();

	get_diagnostics().report_error(location, gmt::concat(feature, L" is not yet supported by the code generator"));
}

void ByteCodeEmitter::emit(SyntaxTree& tree)
{
	visit_syntax_tree(tree);
	emit_pending_instantiations();
	pick_entry_points();
}

bool ByteCodeEmitter::is_entry_point_candidate(MethodSymbol* method)
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

	if (m_entryPointCandidates.front()->get_is_async())
	{
		get_diagnostics().report_error(TextLocation(), L"entry point 'Main' cannot be async");
		return;
	}

	m_image.set_entry_point(m_entryPointCandidates.front());
}

void ByteCodeEmitter::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const FunctionDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	if (is_entry_point_candidate(symbol))
		m_entryPointCandidates.push_back(symbol);

	if ((node->get_body() == nullptr))
		return;

	// open generic definitions are emitted per instantiation, on demand
	if (is_generic_definition(symbol))
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

	std::optional<const OperatorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if ((symbol == nullptr) || (node->get_body() == nullptr))
		return;

	if (is_generic_definition(symbol))
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

	std::optional<const ConstructorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ConstructorSymbol* symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if ((symbol == nullptr) || (node->get_body() == nullptr))
		return;

	if (is_generic_definition(symbol))
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);

	const SyntaxSymbol* parent = symbol->get_parent();
	if (!(parent == nullptr) && parent->is_type())
		emit_field_initializers(gmt::as<StructSymbol>(parent));

	emit_body(node->get_body());
	end_method();
}

void ByteCodeEmitter::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const StructDeclarationSyntax*> nodeRef = ref_from(node);
	StructSymbol* symbol = nodeRef.has_value() ? lookup_symbol<StructSymbol>(*nodeRef) : nullptr;

	// synthesized constructors are never visited by the member visitors: they
	// own no ConstructorDeclaration node, so their body is field initializers
	// only. The symbol-to-node map keeps the first binding, which for them is
	// an arbitrary construction site, not a declaration
	if (!(symbol == nullptr) && symbol->type_parameters.empty())
	{
		for (ConstructorSymbol* constructor : symbol->constructors)
		{
			if ((constructor == nullptr) || constructor->get_calling_convention() != CallingConvention::None)
				continue;

			const SyntaxNode* declaration = get_symbol_table().lookup_node(constructor);
			if (!(declaration == nullptr) && declaration->get_kind() == SyntaxKind::ConstructorDeclaration)
				continue;

			begin_method(constructor);
			emit_field_initializers(symbol);
			end_method();
		}
	}

	SemanticVisitor::visit_struct_declaration(node);
}

void ByteCodeEmitter::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	std::optional<const AccessorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	AccessorSymbol* symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	if (is_generic_definition(symbol))
		return;

	if (!method_supported(symbol, *nodeRef))
		return;

	begin_method(symbol);
	emit_accessor_body(symbol);
	end_method();
}

void ByteCodeEmitter::emit_accessor_body(AccessorSymbol* symbol)
{
	// accessors are semicolon-only: the body is synthesized over the backing field
	const SyntaxSymbol* parent = symbol->get_parent();
	PropertySymbol* property = (parent == nullptr) ? nullptr : gmt::as<PropertySymbol>(parent);
	FieldSymbol* backing = (property == nullptr) ? nullptr : property->get_backing_field();

	if (!(backing == nullptr))
	{
		std::uint32_t fieldSlot = field_slot_index(backing);
		if (property->get_getter() == symbol)
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
}

void ByteCodeEmitter::emit_field_initializers(StructSymbol* ownerType)
{
	for (FieldSymbol* field : ownerType->fields)
	{
		if ((field == nullptr) || field->get_linking() == LINK_STATIC)
			continue;

		const SyntaxNode* declaration = get_symbol_table().lookup_node(field);
		if ((declaration == nullptr) || declaration->get_kind() != SyntaxKind::FieldDeclaration)
			continue;

		const FieldDeclarationSyntax* fieldNode = gmt::as<FieldDeclarationSyntax>(declaration);
		if ((fieldNode->get_expression() == nullptr))
			continue;

		m_encoder.emit_load_arg(0);
		eval_push(sizeof(void*));

		fieldNode->get_expression()->accept(*this);

		m_encoder.emit_store_field(field_slot_index(field));
		eval_pop(2);
	}
}
