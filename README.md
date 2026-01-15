# Quasi
Something like a renderer

## Requirements

- macOS (Metal backend)
- Bazel 7.0+ with bzlmod enabled
- Xcode Command Line Tools (for Metal framework)

## Building

Build the host application:

```bash
bazel build //src/quasi/host:quasi
```

Build the Metal backend:

```bash
bazel build //backends/metal:libquasi_metal.dylib
```

Build everything:

```bash
bazel build //...
```

## Running

Run the demo with the bundled Metal backend:

```bash
bazel run //src/quasi/host:quasi
```

Or specify a backend explicitly:

```bash
bazel run //src/quasi/host:quasi -- /path/to/backend.dylib
```

## Hot Reloading

For hot-reload development, run with an explicit backend path:

```bash
cp bazel-bin/backends/metal/libquasi_metal.dylib /tmp/libquasi_metal.dylib
bazel run //src/quasi/host:quasi -- /tmp/libquasi_metal.dylib
```

Then rebuild and copy the backend while the host is running:

```bash
bazel build //backends/metal:libquasi_metal.dylib && \
  cp bazel-bin/backends/metal/libquasi_metal.dylib /tmp/libquasi_metal.dylib
```

The host will detect the file change and reload the backend automatically.

## Testing

Run all tests:

```bash
bazel test //test/...
```

Run specific tests:

```bash
bazel test //test:async_test
bazel test //test:plugin_test
```

## Project Structure

```
src/quasi/
  async/      - Coroutine scheduler and utilities
  gpu/        - GPU abstraction layer
    metal/    - Metal context and utilities
  host/       - Window management and main application
  plugin/     - Hot-reloadable plugin system

backends/
  metal/      - Metal rendering backend

test/         - Unit tests
```
