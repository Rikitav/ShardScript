# Escape Analysis Responsibility Split

## Intent
Simplify escape analysis and remove placement-policy overlap with layout generation while preserving runtime behavior and all pre-existing working-tree changes.

## Goals
- Escape analysis publishes final local placement verdicts, including array storage and scalar-initializer restrictions currently applied by LayoutGenerator.
- LayoutGenerator consumes those verdicts and only builds argument slots, local slots, scalar field slots, type shapes, and byte counts.
- Consolidate repeated local-type lookup, graph-node initialization, and expression-reference handling where this makes the existing implementation simpler.

## Non-Goals
No new escape rules, changes to graph edge direction, fixpoint convergence, interprocedural summaries, captures, suspension, promotion, or runtime allocation semantics. No broad visitor framework or unrelated cleanup.

## Constraints
The target files already contain user changes. Preserve those changes and the existing C++ conventions and dependencies. Keep allocation-site classification distinct from local-slot normalization: a local's storage restriction must not alter its initializer allocation site's verdict. Preserve escape levels and channels when normalizing placement. Do not commit changes without an explicit request.

## Approaches
1. Selected: centralize final placement policy in escape analysis and simplify layout generation. This removes conflicting ownership and makes published local verdicts match the slots actually generated. Consumers of local verdicts must be checked because they previously saw the pre-normalized decision.
2. Minimal helper extraction: reduces duplication but leaves placement policy split between passes. Not selected because it does not resolve the responsibility overlap.

## Design
Normalize local placement once, when EscapeAnalyzer publishes local verdicts. Move the existing array, primitive, and composite scalar-initializer rules from LayoutGenerator without changing their effective slot behavior. Allocation-site verdicts continue to use the existing graph classification directly.

Use a small shared local-type helper in the pragmatic implementation rather than duplicating variable/parameter casts. Consolidate graph-node initialization and repeated expression-reference-to-graph operations within EscapeAnalyzer; retain the existing storage layout, reference caching, graph traversal order, and algorithm. Avoid extracting helpers solely to rename one line.

LayoutGenerator retains scalar field expansion, receiver/type-shape resolution, synthesized constructor handling, and declaration traversal. It no longer inspects initializer syntax or overrides local placement decisions.

## Verification
Identify and run the repository's existing build and semantic/async test workflows. Add focused coverage using existing test conventions for primitive locals, arrays, uninitialized scalar composites, constructed composites, returned values, and aliased references. Check both published pragmatic verdicts and resulting frame placement where the existing harness supports it. Exercise the interpreter on the reference-semantics scenario and related existing scripts. Compare failures against a pre-refactor baseline and report unrelated failures without fixing them.

## Implementer Decisions
Keep runtime behavior unchanged. Published local verdicts intentionally become the effective final placement formerly chosen privately by LayoutGenerator. Broader method/lambda visitor unification is deferred unless needed for the responsibility split.
