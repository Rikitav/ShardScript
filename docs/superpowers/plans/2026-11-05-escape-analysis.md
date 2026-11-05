# Escape Analysis Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make escape analysis own final placement decisions and make layout generation only construct frames, without changing runtime behavior.

**Architecture:** Normalize local placement when publishing graph results, leaving allocation-site classification untouched. Extract small internal helpers for local type lookup, graph node creation, and container-to-expression edges. Preserve graph storage, traversal, and fixpoint semantics.

**Tech Stack:** C++26, existing GMT containers, CMake/Ninja with clang-cl on Windows, existing ShardScript semantic and async scripts.

**Spec:** `docs/superpowers/specs/2026-11-05-escape-analysis-design.md`

## Global Constraints
- Preserve all pre-existing working-tree changes; implement in the current workspace because those uncommitted changes are part of the required baseline.
- No new dependencies or broad visitor framework.
- Keep local placement restrictions separate from allocation-site verdicts; retain levels and channels.
- No commits without an explicit user request.

## Review Focus
- Parameters remain argument slots and must not acquire variable-slot normalization.
- Array locals always use heap slots, regardless of escape level.
- Primitive locals never undergo scalar field expansion.
- Constructed composites use ordinary stack slots when the raw verdict is Scalar; uninitialized eligible composites retain scalar expansion.
- Aliases, nested lambda captures, returns, and suspension retain existing reference identity and lifetime behavior.

---

### Task 1: Publish Final Local Placement

**Files:**
- Create `ShardScript/src/pragmatic/LocalType.hpp`, an internal inline local-type lookup helper.
- Modify `ShardScript/include/shard/pragmatic/EscapeAnalyzer.hpp` and `ShardScript/src/pragmatic/EscapeAnalyzer_Summaries.cpp` for local-only normalization.
- Modify `ShardScript/include/shard/pragmatic/LayoutGenerator.hpp` and `ShardScript/src/pragmatic/LayoutGenerator.cpp` to remove duplicate policy and local-type lookup.
- Modify `ShardScript/src/pragmatic/EscapeAnalyzer.cpp` and `ShardScript/src/pragmatic/EscapeAnalyzer_Captures.cpp` to reuse the internal type helper.
- Modify the stale post-demotion comment in `ShardScript/src/compilation/ByteCodeEmitter.cpp` without changing emitter behavior.
- Create `tests/semantic/escape_placement.shard`, following existing standalone script conventions.

**Interfaces:**
- Produces internal `StructSymbol* local_type(SyntaxSymbol* local)` with null-safe variable/parameter handling in the shard namespace.
- Produces private `AllocationSiteVerdict classify_local(int id, SyntaxSymbol* local, const gmt::unordered_map<MethodSymbol*, EscapeSummary>& summaries)`.
- Consumes existing `classify_node`, `get_local_verdict`, `get_local_slot`, and frame slot recipes.

- [ ] Build the baseline using `cmd.exe /c build_optimized.bat` and record script outputs before edits. Check for an existing test runner; if none exists, run scripts individually with the built interpreter and explicit timeouts. Preserve existing files in out.
- [ ] Add `escape_placement.shard`: exercise primitive locals, an array, an uninitialized field-only composite, direct construction, a factory return, and alias mutation. Print deterministic values and verify them using the interpreter, following `reference_semantics.shard` conventions.
- [ ] Use an ephemeral C++ probe linked to the built library to analyze representative declarations and assert final published placement equals the corresponding variable slot recipe. Include primitive Stack, array Heap, constructed composite Stack, uninitialized eligible composite Scalar, and unchanged allocation-site verdicts. Run before implementation and confirm the published-verdict mismatch is exposed; do not introduce a new permanent test framework.
- [ ] Implement `local_type` and replace repeated casts in escape analysis, capture materialization, summary publication, and layout generation.
- [ ] Implement `classify_local` by applying the exact existing LayoutGenerator normalization after `classify_node`, skipping parameters. Preserve all other verdict fields. Keep allocation-site publication on `classify_node`.
- [ ] Remove array, primitive, and initializer-based placement overrides from LayoutGenerator. Keep scalar expansion and all slot/shape calculations. Remove now-unused includes and declarations and update stale comments.
- [ ] Rebuild with `cmd.exe /c build_optimized.bat`; rerun the probe and both placement/reference scripts. Expected: assertions pass, reference output remains `55, 56, 77, 78, 79, 5, 79, 20, 10, 30, 40` on separate lines.

### Task 2: Simplify Graph Construction

**Files:**
- Modify `ShardScript/include/shard/pragmatic/EscapeAnalyzer.hpp`.
- Modify `ShardScript/src/pragmatic/EscapeAnalyzer.cpp`.

**Interfaces:**
- Produces private `int add_node(bool allocation)` used by `ensure_local_id` and `ensure_site_id`.
- Produces private `void contain_expression(int containerId, const ExpressionSyntax* expression)` to connect a container to all referenced locals and allocation sites using receiver mode false.
- Existing public interfaces and graph representation remain unchanged.

- [ ] Consolidate initialization of every parallel graph vector into `add_node`; retain local receiver registration in `ensure_local_id` and current-frame ownership in node creation.
- [ ] Extract constructor-argument and collection-element reference-edge loops into `contain_expression`, preserving edge order and null handling. Keep call exposure and argument escape logic unchanged.
- [ ] Remove obsolete commentary and unused includes resulting from this refactor; do not rewrite unrelated visitors or algorithms.
- [ ] Rebuild and rerun placement and reference tests and the temporary metadata probe.
- [ ] Run existing semantic success/error scripts and async scripts individually, comparing exit status and output with the baseline. Include accessors, indexators, nullable references, interface widening, suspended references, and lambda scenarios where available. Existing failures must be reported, not repaired.
- [ ] Review the diff for lost pre-existing edits, graph-node vector parity, accidental allocation-site normalization, stale comments, and public API changes. Run `git diff --check` and `git status --short`, remove only temporary artifacts created by this work, and summarize any verification limitations.
