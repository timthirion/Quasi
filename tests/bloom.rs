//! PT-bloom (plan 0029) integration tests for correctness bugs
//! that the closure-pass code-attacker surfaced but that the
//! inline verification pass in `efe47ac` missed.
//!
//! GPU-dependent tests are `#[ignore]`'d so they don't block CI
//! on adapter-less runners. Run locally with
//! `cargo test --test bloom -- --include-ignored`.

#![cfg(not(target_arch = "wasm32"))]

use quasi::pathtrace::mesh::load_glb_bytes;
use quasi::pathtrace::offscreen::{render_offscreen, BloomConfig, RenderConfig};

const CORNELL_QUADS: &[u8] = include_bytes!("../data/gltf/cornell_quads.gltf");

/// **Code-attacker P0-B regression** — at any resolution below
/// 32 px in either axis the Kawase mip chain would degenerate to
/// a single level, so the downsample loop iterated `0..0`, the
/// upsample loop the same, mip 0 stayed as the raw extract
/// output, and composite added `intensity * extracted_radiance`
/// back into the radiance buffer — brightening pixels above
/// threshold *without any actual blur*. The fix in
/// `src/pathtrace/offscreen.rs` skips the bloom pass entirely
/// (with a `log::warn!`) below the 32-px floor. This test pins
/// that skip by asserting the bloom-on 24×24 render is
/// bit-identical to the bloom-off render.
#[test]
#[ignore]
fn bloom_below_min_dimension_is_skipped_not_wrong() {
    let scene = load_glb_bytes(CORNELL_QUADS).expect("cornell");
    let off = RenderConfig {
        width: 24,
        height: 24,
        samples: 16,
        bloom: None,
        ..RenderConfig::default()
    };
    let on = RenderConfig {
        bloom: Some(BloomConfig::DEFAULT),
        ..off
    };
    let a = render_offscreen(off, &scene);
    let b = render_offscreen(on, &scene);
    assert_eq!(
        a.radiance.len(),
        b.radiance.len(),
        "radiance-buffer length must match"
    );
    for (i, (pa, pb)) in a.radiance.iter().zip(b.radiance.iter()).enumerate() {
        assert_eq!(
            pa, pb,
            "pixel {i}: below the 32-px floor the bloom pass must be a bit-identical no-op; \
             got off={pa:?}, on={pb:?}"
        );
    }
}
