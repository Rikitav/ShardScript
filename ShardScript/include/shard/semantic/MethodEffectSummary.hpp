#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <unordered_set>

namespace shard
{
	class FieldSymbol;
	class StructSymbol;

	struct SHARD_API MethodEffectSummary
	{
		bool mayThrow = false;
		bool mutatesInstance = false;
		bool mutatesStatic = false;
		bool mutatesArguments = false;
		bool hasUnknownSideEffects = false;

		std::unordered_set<gmt::Ref<StructSymbol>> thrownTypes;
		std::unordered_set<gmt::Ref<StructSymbol>> asyncThrownTypes;
		std::unordered_set<gmt::Ref<FieldSymbol>> mayAssignInstanceFields;
		std::unordered_set<gmt::Ref<FieldSymbol>> mayAssignStaticFields;
		std::unordered_set<gmt::Ref<FieldSymbol>> definitelyAssignsInstanceFields;
		std::unordered_set<gmt::Ref<FieldSymbol>> definitelyAssignsStaticFields;

		void merge(const MethodEffectSummary& other);

		bool operator==(const MethodEffectSummary& other) const;
		bool operator!=(const MethodEffectSummary& other) const;
	};
}
