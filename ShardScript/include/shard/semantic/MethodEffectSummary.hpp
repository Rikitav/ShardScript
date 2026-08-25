#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <unordered_set>

namespace shard
{
    class FieldSymbol;
    class TypeSymbol;

    struct SHARD_API MethodEffectSummary
    {
        bool MayThrow = false;
        bool MutatesInstance = false;
        bool MutatesStatic = false;
        bool MutatesArguments = false;
        bool HasUnknownSideEffects = false;

        std::unordered_set<TypeSymbol*> ThrownTypes;
        std::unordered_set<FieldSymbol*> MayAssignInstanceFields;
        std::unordered_set<FieldSymbol*> MayAssignStaticFields;
        std::unordered_set<FieldSymbol*> DefinitelyAssignsInstanceFields;
        std::unordered_set<FieldSymbol*> DefinitelyAssignsStaticFields;

        inline void Merge(const MethodEffectSummary& other)
        {
            MayThrow              = MayThrow              || other.MayThrow;
            MutatesInstance       = MutatesInstance       || other.MutatesInstance;
            MutatesStatic         = MutatesStatic         || other.MutatesStatic;
            MutatesArguments      = MutatesArguments      || other.MutatesArguments;
            HasUnknownSideEffects = HasUnknownSideEffects || other.HasUnknownSideEffects;

            for (TypeSymbol* type : other.ThrownTypes)
                ThrownTypes.insert(type);

            for (FieldSymbol* field : other.MayAssignInstanceFields)
                MayAssignInstanceFields.insert(field);

            for (FieldSymbol* field : other.MayAssignStaticFields)
                MayAssignStaticFields.insert(field);

            for (FieldSymbol* field : other.DefinitelyAssignsInstanceFields)
                DefinitelyAssignsInstanceFields.insert(field);

            for (FieldSymbol* field : other.DefinitelyAssignsStaticFields)
                DefinitelyAssignsStaticFields.insert(field);
        }

        inline bool operator==(const MethodEffectSummary& other) const
        {
            return MayThrow == other.MayThrow
                && MutatesInstance == other.MutatesInstance
                && MutatesStatic == other.MutatesStatic
                && MutatesArguments == other.MutatesArguments
                && HasUnknownSideEffects == other.HasUnknownSideEffects
                && ThrownTypes == other.ThrownTypes
                && MayAssignInstanceFields == other.MayAssignInstanceFields
                && MayAssignStaticFields == other.MayAssignStaticFields
                && DefinitelyAssignsInstanceFields == other.DefinitelyAssignsInstanceFields
                && DefinitelyAssignsStaticFields == other.DefinitelyAssignsStaticFields;
        }

        inline bool operator!=(const MethodEffectSummary& other) const
        {
            return !(*this == other);
        }
    };
}
