# PT-bloom-verification — close the gaps plan 0029 left open

- **Status:** proposed
- **Last updated:** 2026-07-30
- **Last touched on:** created during the 0029 closing pass, to hold the four verification riders and one whole milestone that 0029 shipped without

## Goal

Plan [`0029-pt-bloom`](0029-pt-bloom.md) shipped HDR bloom and it went
straight into gallery-wide use — the Bistro hero, the chess hero, the
Sponza sunlit variant and both Cornell glass scenes are all
bloom-default renders. What it *didn't* ship was most of its own
verification, plus the browser widget surface entirely. This plan
closes that gap so bloom is defensible rather than merely
good-looking, and so it exists on both targets instead of one.

The single most important item is the **widget**. Quasi's stated
constraint — see `plans/README.md`, "Native and web builds are both
first-class: a plan isn't done until it works in both targets" — was
violated by 0029. Bloom is native-only today. Since the whole reason
this renderer exists is the live in-browser widget, a post-process
that only runs offline is a post-process the blog reader never sees.

## Context

What shipped in 0029:

* `src/pathtrace/shaders/bloom_{extract,downsample,upsample,composite}.wgsl`
* the `Option<BloomConfig>` gate and pass sequencing in
  `src/pathtrace/offscreen.rs`
* `--bloom`, `--bloom-intensity`, `--bloom-threshold`, `--bloom-knee`
  in `src/main.rs`, with parse tests
* four CPU unit tests pinning `soft_knee_extract_reference`
* the intensity sweep: seven renders, `examples/analyze_bloom_sweep.rs`,
  `data/output/bloom_intensity_sweep.csv`, and the ratio table in
  0029's `Findings` (locked default `0.04` at 1.8145× ring luminance)

What did not:

* any numeric pin on the **Kawase kernel itself** — the mip chain is
  exercised by every render but its blur characteristics are
  unverified, so a refactor could change the kernel's spread without
  a single test failing
* any assertion that **`--bloom` off is genuinely inert**
* the **denoise × bloom** halo interaction check
* the **widget** toggle + slider

## Milestones

- [ ] **[PT-bloomv/kernel-pin]** CPU test for the Kawase chain: a
  single bright pixel at (128,128) in a 256×256 buffer, run through
  the 4-level chain, asserting
  * total energy within 10% of the input total (the dual-filter
    kernel approximates an energy-conserving Gaussian), and
  * FWHM along the centre row within `[14, 22]` px.

  Needs a CPU reference implementation of the downsample/upsample
  taps mirroring the WGSL, in the same
  `soft_knee_extract_reference` style — the shader stays the source
  of truth for rendering, the CPU mirror is the source of truth for
  the test. Note the mirror can drift from the shader; pin the tap
  weights in one place and reference them from a comment in each.

- [ ] **[PT-bloomv/bypass-assert]** Prove `cfg.bloom == None` is
  inert. Render `cornell_glass_bunny.gltf` at 128×128 / 256 spp /
  PCG / MIS-NEE twice — once on the bloom-off path, once against a
  stored pre-bloom baseline — and assert RMSE < 0.05 over the
  radiance buffer (the threshold used by
  `tests/cornell_gltf.rs`'s `cornell_quads_and_tris_render_to_the_same_image`,
  chosen to catch algorithmic change without tripping on backend FMA
  reordering). Add the `assert_no_bloom_state_touched()` hook so the
  None path panics if any bloom allocation or pass executes.
  Follow the existing `#[ignore]` convention for GPU-requiring tests
  so CI stays deterministic.

- [ ] **[PT-bloomv/halo-interaction]** The check 0029 specified and
  skipped. On the Cornell emissive-sphere scene, run plan 0021's halo
  metric in two **bloom-on** configurations:
  * baseline: `--denoise none --bloom`
  * test: `--denoise atrous --bloom`

  Assert the test metric does not exceed baseline by more than 10%.
  Comparing bloom-on against bloom-on is the load-bearing detail: a
  naive comparison against bloom-off would fail trivially, because
  raising annular-ring luminance is exactly what bloom is *for*. The
  failure mode this actually catches is "the denoiser wakes up an
  unintended halo once bloom is in the loop." Machinery to reuse:
  `halo_intensity_at_ring` in `src/pathtrace/denoise.rs`.

- [ ] **[PT-bloomv/widget]** Bloom on the web target. Widget gains a
  "Bloom" toggle and an intensity slider over `[0.0, 0.15]` centred
  on the locked `0.04` default.
  * **Budget:** ≤ 2 ms per composite on Apple M-series Safari at the
    widget's default 384×288 framebuffer, measured with
    `performance.now()` around the composite call.
  * Debounce on `change` (drag end), not `input` (drag in progress),
    so dragging can't saturate the GPU command queue.
  * The interactive path composites per accumulation step, unlike
    the offline path's single composite before readback — confirm
    that ordering still reads correctly against progressive
    accumulation, and that toggling bloom mid-render doesn't require
    an accumulation reset (it shouldn't: bloom is a post-process on
    the accumulated buffer, not part of the estimator).
  * If the budget is missed, the documented fallback is toggle-only
    (drop the slider), and if it's missed badly, gate bloom off on
    web with the measurement recorded in `Findings` — a negative
    result recorded is a result; silently shipping a janky slider is
    not.

- [ ] **[PT-bloomv/attacker-pass]** Run the `close-plan`
  render-attacker/defender pair in pair-mode on the bloom-default
  gallery renders against their pre-bloom committed versions. Per
  0029's original criterion, the attacker must surface ≥ 1 specific
  halo-improvement region on the Bistro sunlit gothic façade **and**
  ≥ 1 region where bloom did *not* soften an intended-sharp feature
  (cobblestones, awning edges). Findings land here.

## Open questions

* **Does the CPU Kawase mirror earn its keep?** It's duplicated math
  that can drift from the shader. The alternative is a GPU-executed
  test behind `#[ignore]`, which tests the real kernel but doesn't
  run in CI. Leaning CPU mirror for the kernel-pin (cheap, always
  runs, and the tap weights are stable) — but if the mirror needs
  more than ~40 lines, prefer the GPU test.
* **Is 2 ms the right widget budget?** It was asserted in 0029
  without measurement. Measure first, then decide whether 2 ms is
  demanding or generous at 384×288; record the actual number
  regardless of whether it passes.
* **Should the sweep be re-run at widget resolution?** `0.04` was
  locked at 256×256. The mip count caps at 4 levels below 512 px, so
  384×288 gets a different chain depth and possibly a different
  visual weight for the same intensity. If the widget's bloom reads
  differently from the offline renders at the same setting, that's a
  finding worth writing up, not a bug to paper over.

## Done when

* All five milestones ticked
* Kawase kernel numerically pinned; bloom-off inertness asserted
* Halo interaction test green at the ≤ 10% ceiling
* Bloom live in the browser widget with its measured composite cost
  recorded in `Findings` (or a documented, measured decision not to
  ship it on web)
* Attacker pair-mode findings recorded
* Plan moves to `Status: completed`

## Findings

(Populated during execution.)

## Followups (out of scope)

Inherited from 0029, still out of scope here:

* **PT-lens-flare** — anamorphic streaks, ghost reflections; reuses
  the bloom mip chain.
* **PT-exposure** — auto-exposure from the radiance histogram; pairs
  with `PT-sky` so time-of-day changes don't need bloom re-tuning.
* **PT-bloom-physical** — measured/parameterised lens PSF instead of
  the Kawase chain.
* **PT-bloom-aces** — re-sweep the intensity default if an ACES
  tonemap ships. `0.04` is calibrated against Reinhard; a
  compressive tonemap makes the same intensity read weaker.
