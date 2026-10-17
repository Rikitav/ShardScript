#include <shard/pragmatic/LayoutGenerator.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

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

void LayoutGenerator::expand_scalar_local(FrameLayout& layout, gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, std::vector<const TypeShape*>& variableShapes)
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

	std::vector<gmt::Ref<StructSymbol>> argumentTypes;
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

	for (gmt::Ref<StructSymbol> argumentType : argumentTypes)
		layout.add_argument(argumentType);

	layout.set_argument_count(static_cast<std::uint16_t>(argumentTypes.size()));

	const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = method->get_type_parameters();

	std::vector<const TypeShape*> argumentShapes;
	for (gmt::Ref<StructSymbol> argumentType : argumentTypes)
	{
		if (argumentType.is_null())
		{
			argumentShapes.push_back(nullptr);
			continue;
		}

		TypeShape* shape = m_pragmatic.find_or_create_type_shape(model, argumentType);
		argumentShapes.push_back(shape);
	}

	std::vector<const TypeShape*> variableShapes;

	const std::vector<gmt::Ref<SyntaxSymbol>>* order = m_pragmatic.get_local_order(method);
	if (order == nullptr)
	{
		layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes, variableShapes));
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

		bool scalarLocal = verdict.verdict == PlacementVerdict::Scalar;
		scalarLocal = scalarLocal && !localType.is_null();
		scalarLocal = scalarLocal && localType->get_kind() == SyntaxKind::StructDeclaration;
		if (scalarLocal)
		{
			expand_scalar_local(layout, localType, typeParameters, variableShapes);
			continue;
		}

		layout.add_variable(localType, typeParameters, verdict.verdict);

		TypeShape* shape = nullptr;
		if (!localType.is_null())
			shape = m_pragmatic.find_or_create_type_shape(model, localType);

		variableShapes.push_back(shape);
	}

	layout.set_locals_bytes(layout.compute_locals_bytes(argumentShapes, variableShapes));
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
