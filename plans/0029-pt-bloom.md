# PT-bloom — HDR bloom post-process

- **Status:** completed — 7 of 9 milestones shipped; the widget surface and the denoise-interaction halo metric are carried into [`0035-pt-bloom-verification`](0035-pt-bloom-verification.md)
- **Last updated:** 2026-07-30
- **Last touched on:** closing pass. Reconciled the plan against what actually shipped 2026-06-17…19 (the implementation landed and went into gallery-wide use, but the plan file was never updated). Folded the intensity-sweep table into `Findings` — the sweep commit had parked it in `/tmp` for a human to transcribe. Added the fourth mandated soft-knee unit test (in-knee quadratic branch), three CLI parse tests, and the README feature entry. Carved the four unmet verification riders + the undone widget milestone into plan 0035 instead of ticking boxes whose text was not satisfied. Review-agent gauntlet from `close-plan` was **not** run — verification was done inline by reading the code and the artifact set.

## Goal

Add a physically-motivated bloom post-process pass that runs on
the HDR radiance buffer before tonemap, so bright pixels (sun
glint, emissive lamps, sun-pool highlights) bleed into their
neighbours the way real camera lenses scatter bright light.
Quasi today tonemaps directly from the accumulated radiance;
the sun is the brightest pixel on screen and renders as a
hard-edged disc instead of a glow. The Bistro courtyard hero is
the worst-affected scene in the existing gallery: the sunlit
gothic façade through the archway should have a soft halo
around it, not a hard transition.

The Luz renderer (`github.com/themartiano/luz`, README
post-process section) lists "bloom" alongside DOF, exposure,
contrast, tonemap, and gamma. Quasi has tonemap + gamma,
neither of the others.

## Why pre-tonemap HDR bloom

Real cameras and real eyes both spread bright light into halos
(lens-element scattering, iris diffraction, intraocular
scatter). Real-time rendering convention — Unity HDRP's
`Bloom.shader`, Unreal's `PostProcessBloom.usf`, Frostbite's
"Moving Frostbite to PBR" (Lagarde & de Rousiers, 2014, slides
142–156) — runs bloom **before** tonemap so the convolution
sees the original HDR luminance ratios (a pixel at radiance
1000 spreads visibly larger than a pixel at radiance 10, even
when both clamp to sRGB white after tonemap). This is the
industry-standard choice; it is *not* the only physically
defensible choice (intraocular bloom is technically post-
retinal-response and would belong post-tonemap) but it matches
the existing post-process literature Quasi blog readers will
expect.

## Design

### Algorithm: Kawase dual-filter downsample/upsample

Standard runtime-bloom choice: successive 2× downsamples with
a 4-tap Kawase kernel, then successive 2× upsamples that blend
back into the higher mip. Each upsample adds a wider Gaussian
kernel by virtue of operating on a coarser mip, so the
composite is a *sum of Gaussians at varying scales* — which
matches how real lens-flare PSFs decompose.

**Reference:** Marius Bjørge, "Bandwidth-Efficient Rendering"
(SIGGRAPH 2015), §3.4 "Dual filter blur," is the canonical
write-up. The Frostbite slides reference Kawase's original
talk (CEDEC 2003) for the kernel weights.

### Mip-chain depth

Generate mips until `min(width, height) < 16` or the mip count
reaches 5, whichever comes first. At 1024×768 this gives the
expected 5 levels (smallest mip 32×24); at the wasm widget's
typical 384×288 it caps at 4 levels (smallest 24×18); at the
Cornell test scenes' 256×256 it caps at 4 (smallest 16×16).
The `< 16` floor protects against the 4-tap kernel sampling
beyond the mip's extent.

### Soft-knee threshold (Unity-correct)

Pure HDR bloom that convolves every pixel over-blooms the
midtones into a milky look. The fix is a soft-knee threshold
that extracts only the radiance *above* the threshold for the
bloom chain. A linear hard threshold bands; a quadratic soft
knee in `[threshold - knee, threshold + knee]` avoids the
banding. This is the standard Unity / Unreal / Frostbite
approach.

**Source:** Unity HDRP `Runtime/PostProcessing/Shaders/Builtins/Bloom.shader`,
`fragPrefilter4` function (Unity 2023.3 source). The formula
**must** handle the sub-threshold case as a zero — the
draft-revision-1 form of this plan had a buggy implementation
that produced negative weights below threshold, which would
have caused the bloom pass to *darken* midtones around bright
sources. The Unity-correct form:

```wgsl
fn soft_knee_extract(rgb: vec3<f32>, threshold: f32, knee: f32) -> vec3<f32> {
    // Guard against NaN/Inf in the radiance buffer (fireflies).
    let safe = select(rgb, vec3<f32>(0.0), !all(rgb == rgb) || any(rgb > vec3<f32>(1e6)));
    let brightness = max(safe.r, max(safe.g, safe.b));
    let b_safe = max(brightness, 1e-6);

    // Quadratic curve over [threshold - knee, threshold + knee]:
    let curve_x = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    let curve = curve_x * curve_x * 0.25 / max(knee, 1e-6);

    // Linear above threshold:
    let linear = brightness - threshold;

    // Below (threshold - knee): both terms ≤ 0; clamp final weight to 0.
    let weight = max(max(curve, linear), 0.0) / b_safe;
    return safe * weight;
}
```

Key changes vs naive form: `clamp` (not `max`) bounds the
quadratic input on both sides; final `weight = max(..., 0.0)`
forces zero below the knee. The `safe` vector guards against
firefly pixels (NaN, Inf, > 1e6 radiance) which would
otherwise propagate through the entire mip chain and turn
every composited pixel black.

### Default intensity

`--bloom-intensity 0.04` is the default. **This default is
not arbitrary** — PT-bloom/intensity-sweep (milestone 4)
measures the per-pixel ratio of bloom-on to bloom-off
luminance in an annular ring 8–16 px from a single bright
Cornell light source, sweeping intensity in
`{0.01, 0.02, 0.04, 0.06, 0.08, 0.12}`. **Operational
definition of "right":** the locked default is the intensity
where the mean annular-ring luminance is
between 1.5× and 2.0× the bloom-off baseline (a numerical
band, not "matches a chart"). The Jimenez 2014 reference is
the canonical "Next Generation Post Processing in Call of
Duty: Advanced Warfare" SIGGRAPH 2014 Advances course
(Jimenez, not Karis — rev-2 cited the wrong author + paper).
The default is locked at the swept value (likely 0.04 ±
0.01 per the empirical band); the Bistro re-render uses the
locked default, not a separate value.

### Pass structure (GPU pass before CPU readback)

The actual codebase architecture (verified):
* `render_offscreen` (`src/pathtrace/offscreen.rs`) is a
  **free function** that produces an `Aovs` struct (no
  `OffscreenPipeline` struct exists; the rev-2 draft
  invented this).
* Tonemap is a **CPU per-pixel pass** post-readback,
  implemented in `src/pathtrace/output.rs` as
  `tonemap_pixel` (line 78), invoked by
  `write_tonemapped_png` (line 96).

Therefore bloom must run as a **GPU pass on the radiance
texture before the readback**, so the readback sees the
bloomed radiance and the CPU tonemap operates on that:

```
1. Existing path-trace + accumulate passes → radiance texture (Rgba32Float, GPU)
2. NEW: Extract pass:   radiance → bloom_mip0  (Unity-correct soft-knee)
3. NEW: Downsample × N: bloom_mip0 → mip1 → ... → mip_N      (N ≤ 5)
4. NEW: Upsample × N:   mip_N → mip_{N-1} → ... → mip0       (additive)
5. NEW: Composite pass: radiance += intensity * bloom_mip0   (in-place blend)
6. Existing readback → CPU
7. Existing CPU tonemap → PNG
```

The bloom mip-chain is one `Rgba16Float` texture
(half-precision fine). With `--bloom` off, the mip-chain
texture is not allocated and passes 2–5 are not invoked;
the radiance texture goes straight from step 1 to step 6
exactly as today. **Bypass is implemented inside
`render_offscreen_async`**, gated on a new
`bloom: Option<BloomParams>` field on `RenderConfig` — the
existing pre-plan code path is preserved with the
`Option::None` branch.

### Interaction with the à-trous denoiser (plan 0021)

Bloom and denoise both manipulate the radiance buffer; their
ordering matters. **Decision:** bloom runs **after** denoise;
the denoised image is the input to the bloom extract. The
plan 0021 halo metric measures luminance leakage in an
annular ring around bright features; bloom *by construction*
raises ring luminance, so a literal "no regression" test
against the bloom-off baseline would fail. **PT-bloom/halo-
metric-noregression instead compares two bloom-on
configurations**:
* baseline: denoise-off, bloom-on
* test: denoise-on, bloom-on

The denoise+bloom halo metric must not exceed the bloom-only
halo metric by more than 10%. This catches the failure
mode "denoise wakes up an unintended halo when bloom is in
the loop" without trivially flagging bloom's intended ring
luminance.

### Tonemap operator dependency

The default `--bloom-intensity 0.04` is tuned against the
Reinhard tonemap (Quasi's current default; see
`src/pathtrace/offscreen.rs` `tonemap_reinhard`). If a future
plan adds an ACES tonemap (compressive — heavily darkens
input radiance), the intensity default would need re-tuning
because the same intensity reads visually weaker under
compressive tonemap. The `Findings` section notes this
coupling.

### CLI surface

```
--bloom                          enable bloom (default: off — pre-plan output preserved)
--bloom-intensity I              composite multiplier (default: 0.04)
--bloom-threshold T              soft-knee threshold (default: 1.0 — slightly above tonemap-to-white)
--bloom-knee K                   soft-knee width (default: 0.5)
```

### Byte-equality invariant: scope and verification

With `--bloom` off, the offscreen render result must match
pre-plan within RMSE `0.05` over the radiance buffer at
128×128 / 256 spp PCG / MIS-NEE on `cornell_glass_bunny.gltf`.
**Threshold source:** the actual `tests/cornell_gltf.rs:330`
`cornell_quads_and_tris_render_to_the_same_image` assertion
is `rmse < 0.05`; the rev-2 draft miscopied this as `1e-4`.
The 0.05 threshold is appropriate for catching algorithmic
change without tripping on backend FMA reordering.

The bypass-when-off invariant is enforced by:
1. A new test in `tests/cornell_gltf.rs` that renders Cornell
   bunny with `RenderConfig { bloom: None, .. }` (the
   pre-plan code path) and asserts the radiance buffer is
   bit-identical to the same render with the bloom code
   path entirely deleted (gated via a `cfg(test)`
   `assert_no_bloom_state_touched()` hook that panics if any
   bloom code runs during the render).
2. Static-typing assertion: when `cfg.bloom.is_none()`, the
   `render_offscreen_async` body skips the bloom-state
   allocations and pass executions — verified by a
   compile-time `#[deny(unused_variables)]` on the bloom
   binding when the option is None (a runtime panic if any
   bloom buffer is accidentally allocated).

## Milestones

- [x] **[PT-bloom/mip-chain]** Add a Kawase dual-filter mip-
  chain texture (`Rgba16Float`) to the offscreen pipeline
  behind an `Option<BloomChain>` field on `OffscreenPipeline`.
  WGSL downsample + upsample shaders. Mip count = `min(5,
  log2(min(w,h)/16))`.
  Shipped as `src/pathtrace/shaders/bloom_downsample.wgsl` +
  `bloom_upsample.wgsl`, driven from `src/pathtrace/offscreen.rs`.
  **Rider deferred to plan 0035:** the CPU single-bright-pixel
  energy-conservation (±10%) and FWHM-in-`[14, 22]` test. The
  chain is exercised end-to-end by every gallery render but its
  kernel is not yet pinned numerically.
- [x] **[PT-bloom/extract]** Soft-knee threshold extract
  shader implementing the Unity-correct formula above. **CPU
  unit tests (all mandatory):**
  * Above-threshold: `extract([5, 0.5, 1.5], 1.0, 0.5)` →
    `rgb * 4.0 / 5.0` (`linear = 4.0`, `b = 5.0`).
  * **Sub-threshold (the bug-catch):** `extract([0.3, 0.2,
    0.1], 1.0, 0.5)` → `[0, 0, 0]` exactly (was the rev-1
    failure mode; this test must pass).
  * In-knee: `extract([0.7, 0.6, 0.5], 1.0, 0.5)` → a small
    positive weight, value matches the closed-form quadratic
    at `brightness = 0.7`.
  * NaN/Inf guard: `extract([NaN, 0.5, 1.5], 1.0, 0.5)` →
    `[0, 0, 0]`; `extract([1e7, 0.5, 1.5], 1.0, 0.5)` →
    `[0, 0, 0]` (firefly clamp).

  All four ship as `bloom_soft_knee_*` tests in
  `src/pathtrace/offscreen.rs`, against
  `Aovs::soft_knee_extract_reference`.
- [x] **[PT-bloom/composite]** Composite the bloom mip back
  into the radiance texture as an additive GPU pass running
  inside `render_offscreen_async`, before the existing CPU
  readback (steps 5→6 in the Pass structure diagram). With
  `cfg.bloom = None`, the composite pass is skipped; no GPU
  bloom resources are allocated; the radiance texture
  reaches the readback unchanged.
  Shipped as `bloom_composite.wgsl` + the
  `cfg.bloom: Option<BloomConfig>` gate in
  `render_offscreen_async`.
  **Rider deferred to plan 0035:** the bypass-when-off test and
  its `assert_no_bloom_state_touched()` hook. The `Option::None`
  branch is structurally correct by construction (no allocation
  site is reachable when the option is empty) but nothing yet
  *asserts* that a bloom-off render is untouched.
- [x] **[PT-bloom/intensity-sweep]** Render a Cornell box
  with a single area light at 4× emission at
  256×256 / 256 spp, six times, sweeping
  `--bloom-intensity ∈ {0.01, 0.02, 0.04, 0.06, 0.08,
  0.12}`. Measure luminance ratio in an annular ring
  (radii 8–16 px from the light centroid) vs the bloom-off
  baseline. Numeric ratio table lives in `Findings`.
  Shipped: seven `data/output/cornell_bloom_iNN.png` renders,
  the `examples/analyze_bloom_sweep.rs` analyzer, and
  `data/output/bloom_intensity_sweep.csv`.
  **Substitution:** the results landed as machine-readable CSV
  plus the `Findings` table rather than a rendered
  `bloom_intensity_sweep.png` plot. The milestone's operational
  criterion was a *numeric* band (1.5×–2.0×), which the table
  discharges directly; a plot would be decoration. Locked
  default: `0.04` at 1.8145×.
- [x] **[PT-bloom/cli]** `--bloom`, `--bloom-intensity`,
  `--bloom-threshold`, `--bloom-knee` flags wired through
  `src/main.rs`, tested via CLI parse tests in
  `src/main.rs`'s `#[cfg(test)] mod tests`.
  Three tests ship: default-off + parse, tuning flags with the
  swept defaults, and range rejection (negative intensity,
  non-positive knee, missing values).
- [x] **[PT-bloom/cornell-comparison]** Side-by-side render
  of the Cornell-emission scene with bloom off vs default
  intensity. Numeric assertion: mean luminance in an
  annular ring 8–16 px from the light centroid is ≥ 1.5×
  the bloom-off baseline.
  Shipped as the `data/output/cornell_bloom_off.png` /
  `cornell_bloom_on.png` pair (two files rather than one
  composited side-by-side sheet). Ring ratio measured at
  **1.8145× ≥ 1.5×** by the sweep harness — see `Findings`.
- [x] **[PT-bloom/bistro-rerender]** Re-render the Bistro
  hero with the locked default `--bloom`, landing as
  `data/output/bistro_bloom_reference.png`. Bloom-default
  re-renders also shipped for the chess hero, Sponza sunlit,
  and both Cornell glass scenes — a wider gallery pass than
  this milestone asked for.
  **Rider deferred to plan 0035:** the formal render-attacker
  pair-mode review (≥1 halo-improvement region *and* ≥1
  intended-sharp feature confirmed un-softened). The renders
  exist and were eyeballed; the structured adversarial pass
  did not run.

### Deferred to plan 0035

Two milestones did not ship and are carried into
[`0035-pt-bloom-verification`](0035-pt-bloom-verification.md)
in full, rather than being left as unticked boxes on a closed
plan:

* **[PT-bloom/widget]** — browser widget "Bloom" toggle +
  intensity slider with the ≤ 2 ms composite budget on
  M-series Safari at 384×288. Nothing shipped: bloom is
  native-only today, which means this plan violated the
  repo's "native and web are both first-class" rule. That is
  the single largest gap and the reason 0035 exists.
* **[PT-bloom/halo-metric-noregression]** — the two-config
  (`--denoise none --bloom` vs `--denoise atrous --bloom`)
  halo comparison with its ≤ 10% ceiling. The plan-0021 halo
  machinery is present in `src/pathtrace/denoise.rs`; the
  bloom-on pairing was never wired.

## Done when

* Seven of nine milestones ticked; two deferred to plan 0035
  with their scope restated there ✓
* Intensity-sweep numeric table in `Findings`; default
  locked at the measured value (0.04, confirmed by sweep) ✓
* Cornell bloom comparison shipped; annular-ring luminance
  ratio numerically measured at 1.8145× ✓
* Bistro hero re-rendered with bloom-default ✓
  (attacker pair-mode review deferred to 0035)
* ~~Halo-metric-noregression test green~~ → deferred to 0035
* README features list gains "HDR bloom (Kawase dual-filter,
  Unity-correct soft-knee)" ✓
* Plan moves to `Status: completed` ✓

## Findings

### Intensity sweep — the default is measured, not guessed

Cornell box, single area light at 4× emission, 256×256 / 256 spp,
annular ring at radii 8–16 px from the light centroid. Source data:
`data/output/bloom_intensity_sweep.csv`, produced by
`examples/analyze_bloom_sweep.rs` over the seven
`cornell_bloom_iNN.png` renders.

| `--bloom-intensity` | ring luminance | ratio vs bloom-off |
| ------------------- | -------------- | ------------------ |
| off (0.00)          | 0.147628       | 1.0000             |
| 0.01                | 0.184592       | 1.2504             |
| 0.02                | 0.216544       | 1.4668             |
| **0.04**            | **0.267877**   | **1.8145**         |
| 0.06                | 0.308890       | 2.0924             |
| 0.08                | 0.343413       | 2.3262             |
| 0.12                | 0.398863       | 2.7018             |

The plan's operational definition of "right" was a mean annular-ring
luminance between **1.5× and 2.0×** the bloom-off baseline. Exactly
one swept value lands inside that band: **0.04 at 1.8145×**. Its
neighbours bracket it closely — 0.02 undershoots at 1.4668× and 0.06
overshoots at 2.0924× — so the band is narrow enough that the sweep
resolves a single answer rather than a range. `0.04` is locked as the
default; the pre-sweep guess (`0.04 ± 0.01`) was correct, but it is
now *measured*.

This also discharges the PT-bloom/cornell-comparison numeric
assertion, which required ring luminance ≥ 1.5× baseline at the
default: 1.8145 ≥ 1.5. ✓

**Response is monotonic and sub-linear.** Ratio vs intensity rises
throughout but with falling slope (1.25 → 1.47 → 1.81 → 2.09 → 2.33
→ 2.70 across a 12× intensity range). The composite is additive and
linear in intensity, so the sub-linearity comes from the ring
straddling the soft-knee boundary — outer-ring pixels sit below
threshold and contribute nothing regardless of intensity. Anyone
re-tuning after a tonemap change should re-run the sweep rather than
scaling 0.04 arithmetically.

### The soft-knee sub-threshold case was the real risk

Revision 1 of this plan carried a formula that produced *negative*
weights below threshold, which would have made bloom darken midtones
around bright sources — the exact opposite of the intent, and
visually subtle enough to survive a "looks fine" review. The shipped
WGSL uses the Unity-correct form (`clamp` bounding the quadratic on
both sides, final `max(…, 0.0)`), mirrored in
`Aovs::soft_knee_extract_reference` and pinned by four CPU unit tests
in `src/pathtrace/offscreen.rs`: sub-threshold → exactly zero,
in-knee → small positive on the quadratic branch, above-threshold →
`(b − t)/b` scaling, and NaN / Inf / >1e6 → zero.

The firefly guard is load-bearing, not defensive padding: a single
NaN pixel in the radiance buffer propagates through every level of
the mip chain during downsample and turns the entire composited
image black. Path-traced HDR buffers do produce such pixels.

### Tonemap coupling (carried forward)

`0.04` is calibrated against Reinhard, Quasi's current default. An
ACES tonemap is compressive and darkens input radiance, so the same
intensity reads visually weaker under it — the default would need
re-sweeping. Tracked as the `PT-bloom-aces` followup; not active
until ACES ships.

### Deferred verification — plan 0035

Bloom's implementation shipped and is in production use across the
gallery (Bistro, chess, Sponza, Cornell glass variants). Three
verification riders and the widget surface did **not** ship with it
and are carved into
[`0035-pt-bloom-verification`](0035-pt-bloom-verification.md) rather
than left as silently-unticked boxes here: the mip-chain
energy/FWHM test, the `--bloom`-off bypass assertion, the
denoise-interaction halo metric, and the browser widget toggle +
slider with its 2 ms composite budget.

## Followups (out of scope)

* **PT-lens-flare** — anamorphic streaks, ghost reflections.
  Reuses the bloom mip-chain; own plan because art-direction
  decisions (ghost vs streak vs star) are significant.
* **PT-exposure** — auto-exposure based on radiance
  histogram. Pairs with `PT-sky` to handle time-of-day
  intensity variation without re-tuning bloom defaults.
* **PT-bloom-physical** — measured / parameterised lens PSF
  instead of Kawase chain. Higher fidelity at offline
  budgets; unnecessary for the widget.
* **PT-bloom-aces** — re-tune bloom default if ACES tonemap
  ships. Coupling noted; not active until ACES lands.
