# plans/

Living planning documents for Quasi (Rust), versioned in the repo so work can
move between machines without losing state.

## Why these live in the repo

A plan is the hand-off: anyone — human or agent — picking the repo up on another
machine should be able to read the active plan and continue without reconstructing
context. Keep plans current as you work; a ticked checkbox here is the source of
truth for "what's done." Commit plan updates alongside the code they describe.

## Layout

- `ROADMAP.md` — the high-level, phased direction. The north star.
- `NNNN-short-slug.md` — one document per concrete piece of work, zero-padded and
  incrementing (`0001-`, `0002-`, …). The number is ordering, not priority.
  **Next free number: `0036`.**
- `research/` — research plans (hypotheses, experimental designs, paper
  roadmaps) with their own `RNNNN-*` numbering. See `research/README.md`.
  These run in parallel to implementation plans; cross-link freely between
  the two tracks.

## Plan document template

```markdown
# <Title>

- **Status:** proposed | active | blocked | completed | abandoned
- **Last updated:** YYYY-MM-DD
- **Last touched on:** <machine / context, so the next session knows where it ran>

## Goal
One paragraph: what this delivers and why it matters for the roadmap.

## Context
What exists today, relevant files, constraints, prior decisions.

## Design
The approach. Struct sketches, WGSL/pipeline shapes, trade-offs considered.

## Steps
- [ ] Concrete, checkable tasks in order. Tick as you go.

## Open questions
Unresolved decisions. Resolve and record the answer rather than deleting.

## Done when
The acceptance criteria — tests, reference images, perf targets.
```

## Conventions

- Update **Status** and **Last updated** every working session. This is the
  convention that has drifted most in practice: plans 0029 and 0030 both sat at
  a stale status for six weeks while the work moved on around them, and 0029's
  intensity-sweep table sat in `/tmp` waiting to be transcribed. If the plan
  file disagrees with the git log, the plan file is the bug.
- `draft` is not a status — use `proposed`. Plans 0001–0013 use `done` rather
  than `completed` because they shipped before the vocabulary settled; leave
  those headers alone.
- Closing a plan goes through the [`close-plan`](../.claude/skills/close-plan/SKILL.md)
  skill. If a milestone's stated criteria weren't met, split the remainder into
  a follow-up plan rather than ticking the box — see 0029 → 0035 for the worked
  example.
- Resolve an open question in-doc (with the answer) rather than dropping it.
- When a plan is `done`, leave it as a record and link it from `ROADMAP.md`.
- Render-quality work should cite a reference (ground-truth image, paper, metric)
  so correctness is verifiable on any machine.
- Native and web builds are both first-class: a plan isn't done until it works in
  both targets (unless explicitly native-only, e.g. the verification harness).
