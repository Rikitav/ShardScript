#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/TypeShape.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace shard
{
	class SHARD_API TypeShapeCache
	{
		struct KeyHash
		{
			std::size_t operator()(const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& key) const;
		};

		struct KeyEquals
		{
			bool operator()(const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& lhs,
				const std::pair<TypeSymbol*, std::vector<TypeSymbol*>>& rhs) const;
		};

		std::unordered_map<std::pair<TypeSymbol*, std::vector<TypeSymbol*>>, std::unique_ptr<TypeShape>, KeyHash, KeyEquals> _shapes;

		TypeSymbol* SubstituteTypeParameter(TypeSymbol* type,
			const std::vector<TypeParameterSymbol*>& parameters,
			const std::vector<TypeSymbol*>& arguments);

		void BuildShape(TypeShape* shape, TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs);

	public:
		TypeShapeCache() = default;
		TypeShapeCache(const TypeShapeCache&) = delete;
		TypeShapeCache& operator=(const TypeShapeCache&) = delete;

		[[nodiscard]] TypeShape* GetShape(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs) const;

		[[nodiscard]] TypeShape* GetOrCreateShape(TypeSymbol* baseType, const std::vector<TypeSymbol*>& genericArgs);

		[[nodiscard]] inline TypeShape* GetOrCreateShape(TypeSymbol* baseType)
		{
			return GetOrCreateShape(baseType, std::vector<TypeSymbol*>());
		}
	};
}
