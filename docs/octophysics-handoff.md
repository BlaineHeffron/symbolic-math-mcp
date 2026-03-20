# Octophysics Handoff

This note tracks the symbolic-math side of the octophysics integration so work can resume later without reconstructing context from chat logs.

## Current State

- Phase 1 on the symbolic-math side is complete enough for adapter work.
- `christoffel`, `riemann_tensor`, `ricci_tensor`, and `einstein_tensor` support `result_format: "structured"` in addition to the existing human-oriented output.
- Structured output is intended for downstream adapters and includes:
  - `type`
  - `result_format`
  - `rank`
  - `dimensions`
  - `coordinates`
  - `zero`
  - `components`
  - `dense_matrix` for rank-2 tensors (`ricci_tensor`, `einstein_tensor`)
- Structured `components` are now self-sufficient: they emit the full non-zero component set rather than a symmetry-reduced basis.
- Human-format output remains backward compatible.

## Verified Behavior

- Local symbolic-math test suite passed with `ctest --output-on-failure` after the structured-output work.
- Real MCP fixtures were used downstream in `octophysics-stable` to validate parsing for:
  - Christoffel tensors
  - Riemann tensors
  - Ricci tensors with `dense_matrix`
  - Zero Einstein tensors with `dense_matrix`

## Octophysics-Side Follow-On

- A new Dueno thread was bootstrapped for the octophysics side:
  - `thr_8111cebfceb5d96b`
  - title: `octophysics-stable symbolic-math integration next steps`
- The octophysics repo now has a feature-gated parser module for structured tensor output.
- The next intended work is Phase 2 on the octophysics side:
  - add the stdio/MCP client layer
  - add symbolic evaluation support on top of parsed tensor data
  - preserve complexified values and coordinates rather than assuming real-only `f64` evaluation

## Important Constraint For Resuming

- The motion ontology requirement means later numeric evaluation must allow complexified coordinates and values.
- Nothing on the current symbolic-math structured schema needs to change for that requirement because `value` is already a symbolic string field.
- Resume assumption:
  - keep the current parser/schema unchanged unless a concrete downstream need appears
  - push complex-aware evaluation decisions into the octophysics-side Phase 2 layer first

## Complex-Value Support (Completed)

All three follow-on items have been verified and regression-tested:

1. **`evaluate_component` with complex substitutions** — SymEngine's parser handles `I` natively. Substitutions like `"3+2*I"`, `"I"`, and `"1+I"` all work through the existing code path without changes.
2. **Regression tests added** (7 new tests in `test_physics.cpp`):
   - `EvaluateComponentComplexSubstitutions` — multi-variable complex subs
   - `StructuredChristoffelComplexMetricPreservesImaginaryUnit` — `I` preserved in structured output
   - `EvaluateComponentRealBaseline` — real substitution baseline
   - `EvaluateComponentPureImaginary` — `x=I → x²=-1`
   - `EvaluateComponentComplexSquare` — `x=3+2*I → x²+1=6+12*I`
   - `EvaluateComponentSchwarzschildRealHorizon` — g_tt at horizon (real)
   - `EvaluateComponentSchwarzschildComplexR` — g_tt with complex radial coordinate
3. **No MCP schema changes needed** — the `value` field is already a symbolic string, so complex values serialize naturally as strings containing `I`.

## If Work Resumes Here Later

The symbolic-math structured output is confirmed complex-aware. Remaining potential work, if octophysics Phase 2 exposes new needs:

1. Add structured output support to `ricci_scalar` (currently only returns human format).
2. Add structured output support to `covariant_derivative`.
3. If downstream parsing needs canonical complex form (e.g., always `a + b*I`), add a normalization pass.
