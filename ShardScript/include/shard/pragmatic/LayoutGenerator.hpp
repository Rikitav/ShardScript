#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SemanticVisitor.hpp>

#include <shard/pragmatic/PragmaticModel.hpp>

namespace shard
{
	class ConstructorDeclarationSyntax;
	class FunctionDeclarationSyntax;
	class LambdaExpressionSyntax;
	class OperatorDeclarationSyntax;

	// the frame layout pass: consumes the escape verdicts and produces a
	// FrameLayout per analyzed body; the first real consumer of type shapes,
	// triggering their lazy size evaluation
	class SHARD_API LayoutGenerator final : public SemanticVisitor
	{
		PragmaticModel& m_pragmatic;

		void build_frame(gmt::Ref<MethodSymbol> method);
		void expand_scalar_local(FrameLayout& layout, gmt::Ref<StructSymbol> type, const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters, std::vector<const TypeShape*>& variableShapes);
		gmt::Ref<StructSymbol> local_type(gmt::Ref<SyntaxSymbol> local) const;

	public:
		LayoutGenerator(SemanticModel& model, PragmaticModel& pragmatic, DiagnosticsContext& diagnostics);

		LayoutGenerator(const LayoutGenerator& other) = delete;
		LayoutGenerator& operator=(const LayoutGenerator& other) = delete;

		void analyze(SyntaxTree& tree);

	protected:
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
	};
}
