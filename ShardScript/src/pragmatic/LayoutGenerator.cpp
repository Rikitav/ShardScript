#include <shard/pragmatic/LayoutGenerator.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

using namespace shard;

LayoutGenerator::LayoutGenerator(SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
	, m_pragmatic(pragmatic)
{
}

void LayoutGenerator::analyze(SyntaxTree& tree)
{
	visit_syntax_tree(tree);
}

StructSymbol* LayoutGenerator::local_type(SyntaxSymbol* local) const
{
	SyntaxKind kind = local->get_kind();
	if (kind == SyntaxKind::VariableStatement)
		return static_cast<VariableSymbol*>(local)->get_type();

	if (kind == SyntaxKind::Parameter)
		return static_cast<ParameterSymbol*>(local)->get_type();

	return nullptr;
}

void LayoutGenerator::expand_scalar_local(FrameLayout& layout, StructSymbol* type, std::pmr::vector<const TypeShape*>& variableShapes)
{
	SemanticModel& model = get_model();

	for (FieldSymbol* field : type->fields)
	{
		if ((field == nullptr) || (field->get_return_type() == nullptr))
			continue;

		if (field->get_linking() == LINK_STATIC)
			continue;

		StructSymbol* fieldType = field->get_return_type();
		TypeShape* fieldShape = m_pragmatic.find_or_create_type_shape(model, fieldType);

		PlacementVerdict fieldVerdict = fieldShape->is_reference() ? PlacementVerdict::Heap : PlacementVerdict::Stack;
		layout.add_variable(fieldType, fieldVerdict);
		variableShapes.push_back(fieldShape);
	}
}

void LayoutGenerator::build_frame(MethodSymbol* method)
{
	if (method == nullptr)
		return;

	SemanticModel& model = get_model();
	FrameLayout& layout = m_pragmatic.get_or_create_frame_layout(method);

	gmt::inline_vector<StructSymbol*, 4> argumentTypes;
	if (method->get_linking() == LINK_INSTANCE)
	{
		// the receiver slot carries the owning type; accessors are parented to
		// their property, so walk to the enclosing type. Top-level functions
		// own no type and pass a null receiver
		StructSymbol* receiverType = nullptr;
		const SyntaxSymbol* parent = method->get_parent();
		while (!(parent == nullptr))
		{
			if (parent->is_type())
			{
				receiverType = gmt::as<StructSymbol>(parent);
				break;
			}

			parent = parent->get_parent();
		}

		argumentTypes.push_back(receiverType);
	}

	for (ParameterSymbol* parameter : method->get_parameters())
		argumentTypes.push_back(parameter->get_type());

	for (StructSymbol* argumentType : argumentTypes.get_vector())
		layout.add_argument(argumentType);

	const std::uint16_t argumentCount = static_cast<std::uint16_t>(argumentTypes.get_vector().size());
	layout.set_argument_count(argumentCount);

	gmt::inline_vector<const TypeShape*, 8> argumentShapes;
	for (StructSymbol* argumentType : argumentTypes.get_vector())
	{
		if (argumentType == nullptr)
		{
			argumentShapes.push_back(nullptr);
			continue;
		}

		TypeShape* shape = m_pragmatic.find_or_create_type_shape(model, argumentType);
		argumentShapes.push_back(shape);
	}

	gmt::inline_vector<const TypeShape*, 8> variableShapes;

	// hand the resolved shapes to the frame layout so materializing a frame
	// never has to look them up again
	auto publish_shapes = [&]()
	{
		layout.set_argument_shapes(gmt::vector<const TypeShape*>(argumentShapes.get_vector().begin(), argumentShapes.get_vector().end()));
		layout.set_variable_shapes(gmt::vector<const TypeShape*>(variableShapes.get_vector().begin(), variableShapes.get_vector().end()));
	};

	const gmt::vector<SyntaxSymbol*>* order = m_pragmatic.get_local_order(method);
	if (order == nullptr)
	{
		publish_shapes();
		layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes.get_vector(), variableShapes.get_vector()));
		return;
	}

	for (SyntaxSymbol* local : *order)
	{
		if (local == nullptr)
			continue;

		// parameters are caller-owned argument slots, not variable slots
		if (local->get_kind() == SyntaxKind::Parameter)
			continue;

		AllocationSiteVerdict verdict = m_pragmatic.get_local_verdict(method, local);
		StructSymbol* localType = local_type(local);

		// array slots cannot carry their elements inline: an unbounded array
		// type has no fixed payload, so arrays always normalize to heap
		if (!(localType == nullptr) && localType->get_kind() == SyntaxKind::ArrayType)
			verdict.verdict = PlacementVerdict::Heap;

		// demote unconstructible scalars: dematerialized locals cannot run
		// constructors or alias references produced elsewhere. Object-creation,
		// range-literal and call initializers degrade to plain stack placement
		// instead; locals assigned a reference keep heap storage like any
		// other composite
		if (verdict.verdict == PlacementVerdict::Scalar && !(localType == nullptr) && !model.is_primitive_type(localType))
		{
			const SyntaxNode* declaration = get_symbol_table().lookup_node(local);
			bool keepScalar = false;

			if (!(declaration == nullptr) && declaration->get_kind() == SyntaxKind::VariableStatement)
			{
				const VariableStatementSyntax* variableNode = gmt::as<VariableStatementSyntax>(declaration);
				const ExpressionSyntax* initializer = variableNode->get_expression();

				if (initializer == nullptr)
					keepScalar = true;
				else if (initializer->get_kind() == SyntaxKind::ObjectExpression)
					verdict.verdict = PlacementVerdict::Stack;
				else if (initializer->get_kind() == SyntaxKind::RangeExpression)
					verdict.verdict = PlacementVerdict::Stack;
				else if (initializer->get_kind() == SyntaxKind::InvokationExpression)
					verdict.verdict = PlacementVerdict::Stack;
				else
					verdict.verdict = PlacementVerdict::Heap;
			}

			if (!keepScalar && verdict.verdict == PlacementVerdict::Scalar)
				verdict.verdict = PlacementVerdict::Heap;
		}

		// primitives bypass placement machinery entirely: dematerialization is
		// composite-only, so a dead primitive local still gets a plain stack slot
		if (verdict.verdict == PlacementVerdict::Scalar && !(localType == nullptr) && model.is_primitive_type(localType))
			verdict.verdict = PlacementVerdict::Stack;

		bool scalarLocal = verdict.verdict == PlacementVerdict::Scalar;
		scalarLocal = scalarLocal && !(localType == nullptr);
		scalarLocal = scalarLocal && localType->get_kind() == SyntaxKind::StructDeclaration;
		// primitives bypass placement machinery entirely: never dematerialized
		scalarLocal = scalarLocal && !model.is_primitive_type(localType);
		if (scalarLocal)
		{
			m_pragmatic.set_local_slot(method, local, argumentCount + layout.get_variable_count());
			expand_scalar_local(layout, localType, variableShapes.get_vector());
			continue;
		}

		std::uint16_t variableIndex = layout.add_variable(localType, verdict.verdict);
		m_pragmatic.set_local_slot(method, local, argumentCount + variableIndex);

		TypeShape* shape = nullptr;
		if (!(localType == nullptr))
			shape = m_pragmatic.find_or_create_type_shape(model, localType);

		variableShapes.push_back(shape);
	}

	publish_shapes();
	layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes.get_vector(), variableShapes.get_vector()));
}

void LayoutGenerator::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	build_frame(symbol);

	SyntaxVisitor::visit_function_declaration(node);
}

void LayoutGenerator::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	build_frame(symbol);

	SyntaxVisitor::visit_operator_declaration(node);
}

void LayoutGenerator::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ConstructorSymbol* symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	build_frame(symbol);

	SyntaxVisitor::visit_constructor_declaration(node);
}

void LayoutGenerator::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// the anonymous method symbol is bound to the lambda's parameters list node
	MethodSymbol* symbol = nullptr;
	if (!(node->get_parameters() == nullptr))
	{
		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node->get_parameters());
		if (!(bound == nullptr) && bound->is_method())
			symbol = static_cast<MethodSymbol*>(bound);
	}

	build_frame(symbol);

	SyntaxVisitor::visit_lambda_expression(node);
}

void LayoutGenerator::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	AccessorSymbol* symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (!(symbol == nullptr))
		build_frame(symbol);
}

void LayoutGenerator::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	SyntaxVisitor::visit_struct_declaration(node);

	// synthesized constructors own no ConstructorDeclaration node, so no
	// visitor ever reaches them; their frame is field initializers over the
	// receiver only. The symbol-to-node map keeps the first binding, which
	// for them is an arbitrary construction site, not a declaration
	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* symbol = lookup_symbol<StructSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	for (ConstructorSymbol* constructor : symbol->constructors)
	{
		if (constructor == nullptr)
			continue;

		const SyntaxNode* declaration = get_symbol_table().lookup_node(constructor);
		if (!(declaration == nullptr) && declaration->get_kind() == SyntaxKind::ConstructorDeclaration)
			continue;

		build_frame(constructor);
	}
}
