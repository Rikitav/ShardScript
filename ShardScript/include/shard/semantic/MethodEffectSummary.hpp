#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <unordered_set>

namespace shard
{
    class FieldSymbol;

    // =====================================================================
    //  Side-effect / mutation summary for methods, constructors,
    //  operators, accessors and lambdas.
    // =====================================================================
    struct SHARD_API MethodEffectSummary
    {
        // -----------------------------------------------------------------
        //  Diagnostics-relevant bits
        // -----------------------------------------------------------------
        bool MayThrow = false;              // body contains throw or calls something that may throw
        bool MutatesInstance = false;       // writes an instance field/property/indexer of 'this'
        bool MutatesStatic = false;         // writes a static field/property of the enclosing type
        bool MutatesArguments = false;      // writes through by-ref/out parameters (reserved)
        bool HasUnknownSideEffects = false; // calls delegate, virtual/interface target or unannotated extern

        // -----------------------------------------------------------------
        //  Entry points for nullable analysis and constructor late-init
        //  proofs. These sets are intentionally conservative: a field is only
        //  considered "definitely assigned" if it is written on every code
        //  path that returns normally.
        // -----------------------------------------------------------------
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
