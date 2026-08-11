# RT-cluster-cull — GPU-driven cluster culling for the raster track

- **Status:** proposed — rev 2, post first-pass skeptic. No implementation started.
- **Last updated:** 2026-08-11
- **Last touched on:** first substantive `plan-skeptic` pass. Rev 1 was a straight port of the Nanite-direction scoping note and had never been attacked. The skeptic found 3×P0 + 3×P1. Rev 2 addresses each: (P0-1) N × 3100 indirect-draw CPU submission cost added as a new `submission-budget` milestone that gates the stress-scene work — if the microbenchmark busts half the frame budget, the design pivots to material-batched indirect draws before proceeding; (P0-2) stress-scene gains an image-correctness assertion (RMSE ≤ 0.02 vs a naive-draw-all golden), closing the "cull-everything ticks the milestone" hole; (P0-3) the `buffers` milestone stops citing a nonexistent `SceneBuffers` and now correctly names the raster-side `Scene` + `MeshHandle` types the new `RasterClusterBuffers` sits alongside; (P1-4) Hi-Z occlusion gains a camera-discontinuity guard so motum's discrete viewpoint teleports don't produce popping; (P1-5) the sphere test now asserts a max normal-cone half-angle bound so a degenerate Morton-window clusterer can't tick with all-flat cones; (P1-6) the 1 M-tri stress scene is specified as material-uniform (one shared material across all 250 spheres) with the M-material case noted as a follow-up.

## Goal

Add a **GPU-driven cluster culling pipeline** to Quasi's raster
track (`src/raster/`). Subdivide each mesh into ~128-triangle
clusters with bounding spheres + visibility cones; on every
frame, run a compute pass that frustum-culls, backface-cull-
cones, and Hi-Z-occludes clusters, then indirect-draws the
survivors through the existing forward shader.

This is the **tier 1** piece of a Nanite-like virtualized-
geometry track. Two follow-up plans target the harder pieces:
RT-micropoly (software rasterizer for sub-pixel triangles) and
RT-virtualized (cluster DAG + LOD + page streaming). This plan
delivers the GPU-driven-rendering substrate they all sit on,
sized for one focused milestone arc and one blog post.

## Why this is the right tier-1 piece

* **It's the GPU-driven rendering substrate.** Compute-pass
  culling + `dispatchWorkgroupsIndirect` + indirect-draw is
  the architecture every Nanite-style technique builds on.
  Ship this and the next two plans get to focus on their own
  concerns instead of re-inventing the pipeline.
* **It works on WebGPU today.** Compute shaders, atomics on
  storage buffers, and `drawIndexedIndirect` are all in the
  WebGPU MVP. No spec extensions, no Safari-only features, no
  bindless requirement.
* **It serves an actual measurement.** A million-triangle
  stress scene rendering at significantly lower frame time
  than naive draw-all is a publishable "GPU-driven rendering
  in the browser" demo without any of the deeper Nanite
  machinery.
* **It doesn't conflict with motum.** The existing motum-shaped
  JSON scene API continues working unchanged; clusters are
  built per registered mesh at the time of upload, and a
  scene with one cube + one cylinder degenerates to "two
  clusters, both pass culling every frame" — same as today
  in frame-time terms.

## Why this is the wrong place to start (and why we do it anyway)

Quasi's raster track exists to serve motum's in-browser
planner widgets — robots + obstacles, sub-100K-triangle
scenes. Motum will not exercise cluster culling at all in its
typical use case; this plan does not move motum's needle. The
case for shipping it anyway:

1. **The raster track has had R0–R4 done since 2026-06-04 and
   no new plans since.** The track's lifeline as something
   other than "the thing motum uses" needs a forward direction
   that's about *raster as a subject worth writing about*, not
   just *raster as a motum delivery channel*.
2. **The plan-skeptic for RT-micropoly will demand this be
   in place first** (the software rasterizer needs clustered
   input + per-cluster compute dispatch). Better to land it
   as a separable plan with its own validation than as a
   prerequisite buried inside a larger one.

The plan explicitly does **not** lay claim to performance on
small scenes; the stress-scene milestone shows the win at a
scale the existing raster track has not been measured at.

## Design

### Pipeline overview

```
existing per-frame raster (R4):
    ↓
    upload uniforms (camera + scene state)
    draw each registered mesh instance via fixed-function bind groups
    overlay pass (lines, points, goal handle)
    present pass

new per-frame raster:
    ↓
    upload uniforms (camera + scene state)
    [build Hi-Z pyramid from last frame's depth — once, post-resolve]
    cull compute pass: cluster_id → visible-cluster list
    compact visible list into indirect-draw args buffer
    drawIndexedIndirect against the visible-cluster index ranges
    overlay pass (unchanged)
    present pass
```

### Cluster representation

A cluster is **128 triangles** by default (configurable). For
a closed mesh of N triangles, expect ~N/128 clusters. Each
cluster carries:

```rust
#[repr(C)]
struct GpuCluster {
    bounding_sphere: [f32; 4],  // (cx, cy, cz, radius)
    normal_cone: [f32; 4],      // (nx, ny, nz, cos_half_angle)
    index_offset: u32,          // first index in the mesh's index buffer
    index_count: u32,           // always 128 * 3 == 384 except the last cluster
    material_id: u32,
    _pad: u32,
}
```

48 bytes per cluster. A 1 M-triangle mesh produces ~7800
clusters → ~370 KB of cluster metadata. Trivially fits.

### Cluster builder (CPU side)

Two-pass naive clusterer for v1:

1. **Sort triangles by Morton code** of their centroid. Groups
   triangles that are spatially close.
2. **Slide a 128-wide window**, emit each window as a cluster
   with its bounding sphere + normal cone computed over the
   triangles in the window.

This is significantly worse than METIS / Karis's edge-graph
clusterer, but it ships in ~200 LOC, has no external
dependencies, and is good enough for the v1 measurement. A
follow-up (RT-cluster-build-metis) can swap in a real
partitioner once the rest of the pipeline is proven.

Reference for the right clusterer: Brian Karis, "A Deep Dive
into Nanite Virtualized Geometry" (SIGGRAPH 2021), §3.1
"Cluster generation." We'll cite the paper but ship the
Morton-window approximation; the gap is documented in
`Findings`.

### Frustum + backface culling (GPU compute)

One workgroup per 32 clusters (subgroup-sized). Per cluster:

* **Frustum test:** classify the bounding sphere against the
  6 view-frustum planes; out → cull. Standard
  Ericson-style plane-sphere math.
* **Backface cone test:** if the cluster's normal cone's
  half-angle and the cone-from-camera-to-cluster-center
  don't overlap on the camera side, all triangles in the
  cluster face away → cull. Adapted from Karis 2021 §4.2,
  derived more accessibly in the Frostbite "Mesh Shaders"
  presentation (Achton, GDC 2019).

The cull pass writes a compacted visible-cluster index list
into a storage buffer using atomic increment of a counter.

### Hi-Z occlusion culling

Each frame's resolved depth buffer is downsampled into a
mip-chain depth pyramid via a compute pass. The next frame's
cull pass reads the pyramid: for each cluster, sample the
appropriate Hi-Z mip level for the cluster's projected
bounding box; if every sampled texel's depth is *closer to
camera* than the cluster's nearest depth, the cluster is
occluded.

**Standard caveat: this is temporally stable but one-frame
late.** Camera teleports or fast strafes cause one frame of
"too aggressive cull, things pop in." A small expand-bounds
margin (5%) helps for continuous camera motion (mouse-orbit,
scroll-zoom); it does **not** help for the discrete-camera
case, which motum's planner widget hits every time the user
switches viewpoints (skeptic P1-4). A 5% radius expansion on
a 0.1 m cluster gives 5 mm of slack; the last-frame Hi-Z
pyramid is from an entirely different viewpoint and is
worthless for occlusion.

**Camera-discontinuity guard:** the cull pass takes a
uniform `hi_z_valid: u32` flag. Native + widget entry points
compare the new frame's view matrix against the previous
frame's; if the L2 norm of the difference exceeds a threshold
(0.1 in camera-space units for translation, 5° for
rotation — chosen to fire on motum's discrete viewpoint
switches but not on continuous orbit), the flag goes `0` and
the cull pass falls back to frustum + backface only for that
frame. Hi-Z is trusted again on frame `t+1`.

A more robust two-pass approach (cull-by-last-frame, draw,
build Hi-Z, cull-newly-visible, draw again — the Karis 2021
§5 algorithm) eliminates the one-frame-late artifact
entirely at the cost of one extra cull + draw pass per
frame. Documented as `RT-twopass-occlusion` in Followups; the
discontinuity-guard fallback is the widget-ready v1 answer.

### Indirect draw + the WebGPU-MVP API

The cull pass writes one `DrawIndexedIndirectArgs` entry per
surviving cluster into a storage buffer (with a separate
atomic counter for "how many clusters survived"). The CPU
issues N separate `drawIndexedIndirect` calls in a loop,
sourcing each call's args from the GPU-resident buffer.

**CPU-submission budget is load-bearing** (skeptic P0-1). At
7800 clusters × 60 % cull rate = ~3100 surviving clusters per
frame. If each `drawIndexedIndirect` call costs ~10 µs of
Rust→wgpu-hal→Metal (or wgpu→WebGPU→browser JS) overhead —
the realistic range on wgpu 29 is 5–20 µs on Metal, higher in
the browser — then 3100 × 10 µs = 31 ms of CPU submission
*alone*, before the GPU does any work. That would bust the
16 ms native Done-when by 2× and the 33 ms browser Done-when
outright. **The `[RT-cluster-cull/submission-budget]`
milestone measures this in isolation before any stress-scene
work starts.**

Two documented fallbacks if the budget bust:

1. **Material-batched indirect draws** (primary fallback):
   pre-sort the mesh index buffer so triangles of the same
   material are contiguous, then bucket surviving clusters by
   material in the cull pass. Issue **M** draws (M = number
   of materials, ≤ 256 per the texture-array cap), each
   covering a compacted per-material index range. M ≈ 8 on a
   typical scene → ~80 µs of CPU submission, comfortably
   inside budget. Costs one extra compaction pass and a
   material-sorted index buffer at scene-build time.
2. **Batch-all-materials draw** (secondary fallback): if
   material-batched still busts (extremely unlikely), issue
   one `drawIndexedIndirect` per frame covering the entire
   surviving-cluster index range, at the cost of losing
   per-material pipeline switches. Only viable for the
   single-material stress-scene.

The multi-draw-indirect alternative
(`RT-multidraw-indirect`, see Followups) collapses the
N-iteration loop into one call once wgpu surfaces
`WGPUMultiDrawIndirect` on Vulkan/D3D12 backends. WebGPU MVP
doesn't have it; wasm won't get it soon.

The cull pass's output is the survivor count read back to
CPU — a one-frame round-trip we accept as the cost of
WebGPU-MVP scope.

### Bindless workaround for materials

Nanite uses bindless. WebGPU doesn't. Workaround: a **texture
array** (already used by PT-textures in the path-trace track),
material indices in the cluster struct, sampled per fragment.
Same workaround Quasi already ships on the path-trace side.

This caps the renderer at ~256 distinct materials per scene
(texture-array layer limit per spec). The stress scene uses
**one shared material** across all 250 spheres (skeptic P1-6)
so the stress-scene number reflects cull-pass overhead, not
material-switch overhead. The multi-material case
(hundreds of distinct materials in one scene) is a follow-up
(`RT-cluster-cull-multi-material`) and is where the
material-batched indirect-draw fallback earns its keep.

### Native + web lockstep

The cull pass, Hi-Z pyramid, and indirect-draw all use
WebGPU-MVP features. Native + wasm build the same WGSL +
storage-buffer + indirect-draw paths. No conditional code
between targets. The
[`feedback_native_web_lockstep`](../memory/feedback_native_web_lockstep.md)
rule is preserved.

### Backward compatibility with motum

The existing motum-shaped JSON scene API
(`setWorldState` / `setTrajectory` / etc.) registers a few
small meshes; each becomes one cluster (since its tri count
is < 128) and renders identically to today. Existing motum
tests are extended to assert the cluster pipeline doesn't
change the rendered output bit-for-bit (RMSE ≤ 0.05 vs
pre-plan).

## Milestones

- [ ] **[RT-cluster-cull/clusters]** CPU-side mesh clusterer
  in `src/raster/cluster.rs`. Morton-sort-and-window
  algorithm. Per cluster: bounding sphere (Ritter or
  Welzl) and normal cone (Karis 2021 §4.2 formula).
  **CPU unit tests:**
  * On a known unit cube (12 triangles → 1 cluster),
    bounding-sphere centre is origin, radius is `√3/2`,
    normal cone half-angle is `π`.
  * On a flat plane mesh (all normals coincide), normal
    cone half-angle is ~0.
  * On a procedural sphere (5184 triangles → ~41 clusters
    at 128 tri/cluster), every triangle index appears in
    exactly one cluster.
  * **Normal-cone tightness bound** (skeptic P1-5): median
    cluster half-angle on the procedural sphere is
    `≤ π/3` (60°). This gates the Morton-window clusterer
    against a degenerate mode where every cluster's cone
    covers the whole hemisphere and the backface cull
    contributes nothing. If the bound trips, the plan
    switches to `RT-cluster-build-metis` before proceeding
    to the frustum-cull milestone.
- [ ] **[RT-cluster-cull/buffers]** GPU upload path:
  `GpuCluster` storage buffer + per-cluster vertex/index
  buffers. New `RasterClusterBuffers` struct in
  `src/raster/cluster.rs`, sitting alongside the existing
  `Scene` + `MeshHandle` types in `src/raster/scene.rs`
  (skeptic P0-3 correction — the previous milestone text
  cited a `SceneBuffers` that doesn't exist in the raster
  module; the closest analogue is the path-trace-side
  `SceneBuffers` in `src/pathtrace/scene.rs`, which
  motivates the naming but isn't imported here). Existing
  `upload_mesh` keeps its contract; the cluster builder
  runs alongside the current `MeshHandle` path so the
  motum-facing API is unaffected.
- [ ] **[RT-cluster-cull/frustum-cull]** WGSL compute shader
  `src/raster/shaders/cull.wgsl`. One workgroup per 32
  clusters; per-cluster plane-sphere classification against
  6 frustum planes. Writes `1` or `0` to a per-cluster
  visibility byte buffer. **Test:** known camera pointing
  at a procedural sphere, with another sphere placed off-
  screen → only the on-screen sphere's clusters survive,
  numeric count matches the closed-form expectation.
- [ ] **[RT-cluster-cull/backface-cone]** Adds backface-cone
  rejection to the cull shader. **Test:** a back-facing
  cluster (all triangles face away from the camera) gets
  culled; flipping the camera 180° flips which clusters
  survive. Tested on the procedural sphere.
- [ ] **[RT-cluster-cull/hi-z]** Hi-Z pyramid build pass
  (`src/raster/shaders/hiz.wgsl`) + occlusion test in
  `cull.wgsl`. Pyramid is a mip-chain of the depth buffer;
  each mip stores the **max** of its 4 source texels
  (reverse-Z assumed; depth=1 means far). **Test:** a small
  cluster placed behind a large occluder gets culled at the
  appropriate Hi-Z mip level. Numeric: the cull-rate on a
  stress scene with high overdraw is ≥ 50%.
- [ ] **[RT-cluster-cull/indirect-draw]** Compaction pass
  writes `DrawIndexedIndirectArgs` to a storage buffer;
  surviving-count read back to CPU; CPU issues N
  `drawIndexedIndirect` calls. **Correctness test:** on a
  simple procedural scene (~ 40 clusters, one material),
  the rendered frame with `cull-and-draw` matches the
  naive `draw-all-clusters` render within RMSE ≤ 0.001
  (essentially identical; only difference is indirect-draw
  command ordering). Golden PNG committed at
  `data/output/cluster_cull_indirect_ref.png`.
- [ ] **[RT-cluster-cull/submission-budget]** *New in rev 2
  (skeptic P0-1).* Microbenchmark the CPU cost of N
  `drawIndexedIndirect` calls in isolation at
  `N ∈ {100, 500, 1000, 3000, 8000}`, no cull pass, just
  submit-and-time. Writes results to
  `data/output/indirect_draw_submission_budget.csv`. **Hard
  gate:** if `3000 × per-call-cost > 8 ms` on Apple M-series
  native (i.e. more than half the 16 ms frame budget goes to
  submission alone), the plan pivots to the material-batched
  indirect-draw fallback before starting the stress-scene
  milestone. If wasm/Safari overhead is more than 3× native,
  the widget path documents itself as material-batched-only.
  The CSV lands in `Findings`; the pivot decision is recorded
  there too, whether the gate fired or not.
- [ ] **[RT-cluster-cull/motum-noregression]** Re-run the
  existing motum-API tests with the cluster pipeline
  enabled. **Done-when:** existing motum scene tests pass
  unchanged; rendered output matches the pre-plan
  rendering within RMSE ≤ 0.05. **Special case for the
  Hi-Z camera-discontinuity guard:** the motum tests
  include one viewpoint-teleport transition (frame N at
  pose A, frame N+1 at pose B); the RMSE assertion on
  frame N+1 must hold — no popping-in — which pins the
  discontinuity guard (skeptic P1-4).
- [ ] **[RT-cluster-cull/stress-scene]** Procedural scene
  with 1 M total triangles (250 subdivided spheres at
  4000 triangles each, scattered through a viewing volume,
  **all sharing one material** per the P1-6 correction).
  **Numeric Done-when:**
  * Frame time with cluster culling enabled: ≤ 16 ms at
    1280×720 on Apple M-series native.
  * Frame time with naive draw-all (no culling): ≥ 80 ms
    on the same hardware (i.e. ≥ 5× speedup from culling).
  * Cull rate (fraction of clusters culled per frame): ≥ 60%
    on a typical viewpoint.
  * **Image correctness (skeptic P0-2):** rendered frame
    matches a naive-draw-all reference of the same 1 M-tri
    scene within RMSE ≤ 0.02. The reference is rendered
    offline once (with culling disabled) and committed as
    `data/output/cluster_stress_reference.png`. This
    assertion closes the "a cull shader that culls
    everything ticks the frame-time criteria trivially"
    hole — a broken cull that produces a black frame or
    dropped clusters fails the RMSE bound loudly.
  * Same scene rendered in browser (Apple M-series Safari):
    ≤ 33 ms (30 fps target for the browser, which carries a
    WebGPU driver overhead the native path doesn't pay).

## Done when

* All nine milestones ticked (rev 2 adds `submission-budget` as
  the ninth, gating the stress-scene work)
* Submission-budget CSV in `Findings`; the material-batched
  pivot decision is documented one way or the other
* Stress-scene numeric table in `Findings`: M-series native
  frame time, M-series Safari frame time, cull rate, image
  RMSE vs the golden, on a reproducible procedural scene
  committed at `examples/gen_cluster_stress.rs`
* Motum existing tests pass with cluster pipeline enabled;
  the Hi-Z discontinuity-guard case (viewpoint teleport) is
  green
* README features list gains "GPU-driven cluster culling
  (RT-cluster-cull)" under runtime
* Plan moves to `Status: completed`

## Findings

(Populated during execution.)

## Followups (out of scope)

* **RT-cluster-build-metis** — swap the Morton-window
  clusterer for a proper edge-graph partitioner (METIS or
  the Karis 2021 algorithm). Yields more spherical
  clusters → tighter bounds → higher cull rates. Gate:
  triggered *inside* rev 2 if the sphere-test half-angle
  bound trips (`> π/3`).
* **RT-cluster-cull-multi-material** — the stress-scene
  milestone constrains itself to one material to isolate
  cull-pass cost from material-switch cost (skeptic P1-6).
  A follow-up scene with hundreds of distinct materials
  exercises the material-batched indirect-draw path
  end-to-end and measures the per-material draw-call
  submission tax on both native and browser targets.
* **RT-multidraw-indirect** — once wgpu surfaces
  multi-draw-indirect on Vulkan/D3D12 backends, collapse
  the N-iteration CPU loop into one call. WebGPU MVP gap;
  doesn't help wasm. Whether this earns its keep depends
  on whether the material-batched fallback already brought
  N low enough — record in `Findings`.
* **RT-twopass-occlusion** — Karis 2021 §5's two-pass
  occlusion (draw-last-visible, build Hi-Z,
  cull-newly-visible, draw again) eliminates the
  one-frame-late artifact entirely at the cost of one
  extra cull + draw pass per frame. The rev-2 Hi-Z
  camera-discontinuity guard is the v1 answer; this
  follow-up is the v2 answer if the guard's fallback-to-
  frustum-only frame produces visible artifacts on real
  motum widget usage.
* **RT-cluster-lod** — per-cluster LOD selection at draw
  time. Prerequisite for RT-virtualized; meaningful standalone
  if scene-scale geometry warrants.
* **RT-micropoly** (plan 0033) — software rasterizer for
  sub-pixel triangles. Plugs into the indirect-draw arg
  buffer with a per-cluster "small or large" classifier.
  Note: 0033 depends on this plan's cluster-id and
  indirect-arg buffer layouts being stable — the
  `submission-budget` pivot to material-batched draws (if
  it fires) changes those layouts, so 0033 waits for rev
  2's stress-scene work to complete before opening.
* **RT-virtualized** (plan 0034) — cluster DAG + LOD +
  streaming. The full Nanite story.
