#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
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

		gmt::unordered_set<gmt::Ref<StructSymbol>> thrownTypes;
		gmt::unordered_set<gmt::Ref<StructSymbol>> asyncThrownTypes;
		gmt::unordered_set<gmt::Ref<FieldSymbol>> mayAssignInstanceFields;
		gmt::unordered_set<gmt::Ref<FieldSymbol>> mayAssignStaticFields;
		gmt::unordered_set<gmt::Ref<FieldSymbol>> definitelyAssignsInstanceFields;
		gmt::unordered_set<gmt::Ref<FieldSymbol>> definitelyAssignsStaticFields;

		void merge(const MethodEffectSummary& other);

		bool operator==(const MethodEffectSummary& other) const;
		bool operator!=(const MethodEffectSummary& other) const;
	};
}
