#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <shard/runtime/TypeShapeCache.hpp>

#include <memory>
#include <functional>

namespace shard
{
	class MethodSymbol;
	class ConstructorSymbol;
	class OperatorSymbol;
	class FieldSymbol;
	class PropertySymbol;
	class IndexatorSymbol;
	class TypeParameterSymbol;
	class GenericTypeSymbol;
	class CallStackFrame;

	class SHARD_API SemanticModel
	{
	public:
		shard::SyntaxTree& Tree;
		std::unique_ptr<shard::SymbolTable> Table;
		std::unique_ptr<shard::NamespaceTree> Namespaces;
		std::unique_ptr<shard::TypeShapeCache> TypeShapes;

		SemanticModel(shard::SyntaxTree& tree);
		~SemanticModel();

		SemanticModel(const SemanticModel&) = delete;
		SemanticModel& operator=(const SemanticModel&) = delete;

		// =========================================================================
		//  Type system utilities
		// =========================================================================
		static bool AreTypesEqual(const TypeSymbol* left, const TypeSymbol* right);
		static bool IsAssignableTo(const TypeSymbol* target, const TypeSymbol* source);
		static bool IsPrimitiveType(const TypeSymbol* type);
		static std::wstring GetTypeDisplayName(const TypeSymbol* type);

		static TypeSymbol* SubstituteType(TypeSymbol* type, GenericTypeSymbol* within);
		static TypeSymbol* GetFieldType(FieldSymbol* field, GenericTypeSymbol* within = nullptr);
		static TypeSymbol* GetMethodReturnType(MethodSymbol* method, GenericTypeSymbol* within = nullptr);
		static TypeSymbol* GetPropertyType(PropertySymbol* property, GenericTypeSymbol* within = nullptr);
		static TypeSymbol* GetConstructorReturnType(ConstructorSymbol* constructor, GenericTypeSymbol* within = nullptr);

		// =========================================================================
		//  Symbol lookup helpers
		// =========================================================================
		static TypeSymbol* FindTypeByName(SymbolTable* table, const std::wstring& fullName);
		static FieldSymbol* FindFieldByName(TypeSymbol* type, const std::wstring& name);

		// =========================================================================
		//  Runtime generic argument resolution
		// =========================================================================
		using TypeParameterResolver = std::function<TypeSymbol*(TypeParameterSymbol*)>;

		static TypeSymbol* ResolveRuntimeTypeArgument(TypeSymbol* type, const TypeParameterResolver& resolveParameter);
		static bool TryResolveGenericArguments(TypeSymbol* type, const TypeParameterResolver& resolveParameter, TypeSymbol*& baseType, std::vector<TypeSymbol*>& genericArgs);
	};
}
