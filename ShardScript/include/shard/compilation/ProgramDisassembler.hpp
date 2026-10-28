#pragma once
#include <shard/Definitions.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Containers.hpp>

#include <ostream>
#include <vector>

namespace shard
{
	class SHARD_API ProgramDisassembler final : private SyntaxVisitor
	{
		const ApplicationDomain& m_domain;
		gmt::vector<MethodSymbol*> m_foundMethods;

		template<typename T>
		T* lookup_symbol(const SyntaxNode* node);

		void disassemble_method(std::wostream& out, MethodSymbol* method);

	public:
		ProgramDisassembler(const ApplicationDomain& domain);
		~ProgramDisassembler() = default;

		void disassemble(std::wostream& out);

		void visit_accessor_declaration(const AccessorDeclarationSyntax* node) override;
		void visit_constructor_declaration(const ConstructorDeclarationSyntax* node) override;
		void visit_operator_declaration(const OperatorDeclarationSyntax* node) override;
		void visit_function_declaration(const FunctionDeclarationSyntax* node) override;
		void visit_lambda_expression(const LambdaExpressionSyntax* node) override;
	};
}
