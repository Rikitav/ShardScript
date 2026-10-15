#include <shard/semantic/MethodEffectSummary.hpp>

using namespace shard;

void MethodEffectSummary::merge(const MethodEffectSummary& other)
{
	mayThrow = mayThrow || other.mayThrow;
	mutatesInstance = mutatesInstance || other.mutatesInstance;
	mutatesStatic = mutatesStatic || other.mutatesStatic;
	mutatesArguments = mutatesArguments || other.mutatesArguments;
	hasUnknownSideEffects = hasUnknownSideEffects || other.hasUnknownSideEffects;

	for (gmt::Ref<StructSymbol> type : other.thrownTypes)
		thrownTypes.insert(type);

	for (gmt::Ref<StructSymbol> type : other.asyncThrownTypes)
		asyncThrownTypes.insert(type);

	for (gmt::Ref<FieldSymbol> field : other.mayAssignInstanceFields)
		mayAssignInstanceFields.insert(field);

	for (gmt::Ref<FieldSymbol> field : other.mayAssignStaticFields)
		mayAssignStaticFields.insert(field);

	for (gmt::Ref<FieldSymbol> field : other.definitelyAssignsInstanceFields)
		definitelyAssignsInstanceFields.insert(field);

	for (gmt::Ref<FieldSymbol> field : other.definitelyAssignsStaticFields)
		definitelyAssignsStaticFields.insert(field);
}

bool MethodEffectSummary::operator==(const MethodEffectSummary& other) const
{
	const bool flagsEqual = mayThrow == other.mayThrow
		&& mutatesInstance == other.mutatesInstance
		&& mutatesStatic == other.mutatesStatic;

	const bool effectsEqual = mutatesArguments == other.mutatesArguments
		&& hasUnknownSideEffects == other.hasUnknownSideEffects;

	return flagsEqual
		&& effectsEqual
		&& thrownTypes == other.thrownTypes
		&& asyncThrownTypes == other.asyncThrownTypes
		&& mayAssignInstanceFields == other.mayAssignInstanceFields
		&& mayAssignStaticFields == other.mayAssignStaticFields
		&& definitelyAssignsInstanceFields == other.definitelyAssignsInstanceFields
		&& definitelyAssignsStaticFields == other.definitelyAssignsStaticFields;
}

bool MethodEffectSummary::operator!=(const MethodEffectSummary& other) const
{
	return !(*this == other);
}
