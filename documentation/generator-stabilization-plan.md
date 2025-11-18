# Generator Stabilization Plan — Java, Lua, Python

Goal

- Ship stable, production-ready Java/Lua/Python generators that reach parity with C++ and integrate cleanly into the drop-in system.

Scope & Definition of Done

- Feature parity for type mappings, template outputs, and drop-in behavior for JSON + DB generators.
- Unit and integration tests (golden files) for each generator, covering templates, interop cases, and edge-cases.
- Example multi-language stack for each generator: (C++ + MySQL + Java), (C++ + SQLite + Lua), (C++ + JSON + Python).
- CI coverage with regression checks and performance benchmarks.
- Documentation for generator usage, migration, and security (plugin signing).

Timeline (3 sprints recommended)

- Sprint 1: Triage & Core Parity
  - Triage current generator limitations and missing templates.
  - Complete type mappings for primitive and complex types; ensure `id` injection behavior is consistent.
  - Add unit tests for simple structs, enums, arrays, and references.
  - Owner: Generator devs.

- Sprint 2: Integration & Drop-Ins
  - Implement drop-in injection parity: JSON/DB methods injected into Java/Lua/Python classes match C++ behavior.
  - Add integration tests for combined generator combos: JSON+Java, SQLite+Lua, MySQL+Python.
  - Add golden-file tests for codegen outputs and SQL.
  - Owner: Generator & integration devs.

- Sprint 3: Hardening & Delivery
  - Add performance benchmarks and optimize template outputs that are bottlenecks.
  - Add plugin security documentation and signing checks for dynamic generators.
  - Add comprehensive docs, sample stacks, and release packaging (binaries/Docker).
  - Run UX check: LSP & debugger experience for the new generators.
  - Create release notes and migration guidance.
  - Owner: Dev leads & docs.

Testing Strategy

- Unit Tests: Validate type mapping logic, template callbacks, generator CLI flags.
- Golden-File Tests: Rendered code and SQL compared to expected output for a curated suite of schemas.
- Interop Tests: End-to-end stack generation for common combos; ensure cross-language bridges (JNI, exec wrappers) compile and run.
- Performance Tests: Generate benchmarks for hot paths (serialization, DB row creation/iteration).
- Security Tests: Validate plugin loading behavior, policy enforcement, and signing mechanics.

CI & Release

- CI runs full generator tests on push + PRs; golden-file tests run in a nightly job with stricter acceptance rules.
- Release CI produces a signed release artifact with binaries and Docker images for the transpiler and example stacks.

Documentation & Examples

- Add `documentation/generator-stabilization-plan.md` (this doc), plus per-generator README (JavaGenerator.md, LuaGenerator.md, PythonGenerator.md) describing:
  - How to enable and use the generator,
  - Differences/edge cases vs C++ generator,
  - Migration examples and sample stacks.

Risk Management

- Risk: Unexpected template drift between languages.
  - Mitigation: Tight coupling between golden-file tests and CI. Fail PRs where golden files change unexpectedly.
- Risk: JNI complexity for C++↔Java interop.
  - Mitigation: Minimal runtime bridge code first, then optimize on performance once tests pass.
- Risk: Dynamic plugin compatibility across OS/arch.
  - Mitigation: Publish binary compatibility matrix and ensure CI covers common target platforms.

Final Acceptance Criteria

- All tests for each generator pass in CI.
- Three sample cross-language stacks are documented and build successfully.
- Release artifacts published and signed, with consumer-friendly docs and migration guides.

Open Actions After Release

- Add more language generators (Rust, Go) and persistent sample stacks.
- Host a plugin registry and signing workflow for external generator authors.
- Collect adoption data and add feedback-driven roadmap for features.
