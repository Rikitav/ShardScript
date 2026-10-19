#include <shard/pragmatic/LayoutGenerator.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

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

gmt::Ref<StructSymbol> LayoutGenerator::local_type(gmt::Ref<SyntaxSymbol> local) const
{
	SyntaxKind kind = local->get_kind();
	if (kind == SyntaxKind::VariableStatement)
		return local.as<VariableSymbol>()->get_type();

	if (kind == SyntaxKind::Parameter)
		return local.as<ParameterSymbol>()->get_type();

	return gmt::nullref;
}

void LayoutGenerator::expand_scalar_local(FrameLayout& layout, gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, std::pmr::vector<const TypeShape*>& variableShapes)
{
	SemanticModel& model = get_model();

	for (gmt::Ref<FieldSymbol> field : type->fields)
	{
		if (field.is_null() || field->get_return_type().is_null())
			continue;

		if (field->get_linking() == LINK_STATIC)
			continue;

		gmt::Ref<StructSymbol> fieldType = field->get_return_type();
		TypeShape* fieldShape = m_pragmatic.find_or_create_type_shape(model, fieldType);

		PlacementVerdict fieldVerdict = fieldShape->is_reference() ? PlacementVerdict::Heap : PlacementVerdict::Stack;
		layout.add_variable(fieldType, typeParameters, fieldVerdict);
		variableShapes.push_back(fieldShape);
	}
}

void LayoutGenerator::build_frame(gmt::Ref<MethodSymbol> method)
{
	if (method.is_null())
		return;

	SemanticModel& model = get_model();
	FrameLayout& layout = m_pragmatic.get_or_create_frame_layout(method);

	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> argumentTypes;
	if (method->get_linking() == LINK_INSTANCE)
	{
		// the receiver slot carries the owning type; top-level functions own no type and pass a null receiver
		gmt::Ref<StructSymbol> receiverType;
		gmt::Ref<const SyntaxSymbol> parent = method->get_parent();
		if (!parent.is_null() && parent->is_type())
			receiverType = parent.as<StructSymbol>();

		argumentTypes.push_back(receiverType);
	}

	for (gmt::Ref<ParameterSymbol> parameter : method->get_parameters())
		argumentTypes.push_back(parameter->get_type());

	for (gmt::Ref<StructSymbol> argumentType : argumentTypes.get_vector())
		layout.add_argument(argumentType);

	const std::uint16_t argumentCount = static_cast<std::uint16_t>(argumentTypes.get_vector().size());
	layout.set_argument_count(argumentCount);

	const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = method->get_type_parameters();

	gmt::inline_vector<const TypeShape*, 8> argumentShapes;
	for (gmt::Ref<StructSymbol> argumentType : argumentTypes.get_vector())
	{
		if (argumentType.is_null())
		{
			argumentShapes.push_back(nullptr);
			continue;
		}

		TypeShape* shape = m_pragmatic.find_or_create_type_shape(model, argumentType);
		argumentShapes.push_back(shape);
	}

	gmt::inline_vector<const TypeShape*, 8> variableShapes;

	const std::vector<gmt::Ref<SyntaxSymbol>>* order = m_pragmatic.get_local_order(method);
	if (order == nullptr)
	{
		layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes.get_vector(), variableShapes.get_vector()));
		return;
	}

	for (gmt::Ref<SyntaxSymbol> local : *order)
	{
		if (local.is_null())
			continue;

		// parameters are caller-owned argument slots, not variable slots
		if (local->get_kind() == SyntaxKind::Parameter)
			continue;

		AllocationSiteVerdict verdict = m_pragmatic.get_local_verdict(method, local);
		gmt::Ref<StructSymbol> localType = local_type(local);

		// array slots cannot carry their elements inline: an unbounded array
		// type has no fixed payload, so arrays always normalize to heap
		if (!localType.is_null() && localType->get_kind() == SyntaxKind::ArrayType)
			verdict.verdict = PlacementVerdict::Heap;

		// demote unconstructible scalars: dematerialized locals cannot run
		// constructors or alias references produced elsewhere. Object-creation
		// and range-literal initializers degrade to plain stack placement
		// instead; locals assigned a reference keep heap storage like any
		// other composite
		if (verdict.verdict == PlacementVerdict::Scalar && !localType.is_null() && !model.is_primitive_type(localType))
		{
			gmt::Ref<const SyntaxNode> declaration = get_symbol_table().lookup_node(local);
			bool keepScalar = false;

			if (!declaration.is_null() && declaration->get_kind() == SyntaxKind::VariableStatement)
			{
				const VariableStatementSyntax* variableNode = declaration.as<VariableStatementSyntax>().as_ptr();
				gmt::Ref<const ExpressionSyntax> initializer = variableNode->get_expression();

				if (initializer.is_null())
					keepScalar = true;
				else if (initializer->get_kind() == SyntaxKind::ObjectExpression)
					verdict.verdict = PlacementVerdict::Stack;
				else if (initializer->get_kind() == SyntaxKind::RangeExpression)
					verdict.verdict = PlacementVerdict::Stack;
				else
					verdict.verdict = PlacementVerdict::Heap;
			}

			if (!keepScalar && verdict.verdict == PlacementVerdict::Scalar)
				verdict.verdict = PlacementVerdict::Heap;
		}

		bool scalarLocal = verdict.verdict == PlacementVerdict::Scalar;
		scalarLocal = scalarLocal && !localType.is_null();
		scalarLocal = scalarLocal && localType->get_kind() == SyntaxKind::StructDeclaration;
		// primitives bypass placement machinery entirely: never dematerialized
		scalarLocal = scalarLocal && !model.is_primitive_type(localType);
		if (scalarLocal)
		{
			m_pragmatic.set_local_slot(method, local, argumentCount + layout.get_variable_count());
			expand_scalar_local(layout, localType, typeParameters, variableShapes.get_vector());
			continue;
		}

		std::uint16_t variableIndex = layout.add_variable(localType, typeParameters, verdict.verdict);
		m_pragmatic.set_local_slot(method, local, argumentCount + variableIndex);

		TypeShape* shape = nullptr;
		if (!localType.is_null())
			shape = m_pragmatic.find_or_create_type_shape(model, localType);

		variableShapes.push_back(shape);
	}

	layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes.get_vector(), variableShapes.get_vector()));
}

void LayoutGenerator::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
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

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
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

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	build_frame(symbol);

	SyntaxVisitor::visit_constructor_declaration(node);
}

void LayoutGenerator::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// the anonymous method symbol is bound to the lambda's parameters list node
	gmt::Ref<MethodSymbol> symbol;
	if (!node->get_parameters().is_null())
	{
		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(node->get_parameters());
		if (!bound.is_null() && bound->is_method())
			symbol = bound.as<MethodSymbol>();
	}

	build_frame(symbol);

	SyntaxVisitor::visit_lambda_expression(node);
}
