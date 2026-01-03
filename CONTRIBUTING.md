# Contributing to Limcode

We love your input! We want to make contributing to Limcode as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## We Develop with GitHub

We use GitHub to host code, to track issues and feature requests, as well as accept pull requests.

## We Use [GitHub Flow](https://guides.github.com/introduction/flow/index.html)

Pull requests are the best way to propose changes to the codebase:

1. Fork the repo and create your branch from `master`.
2. If you've added code that should be tested, add tests.
3. If you've changed APIs, update the documentation.
4. Ensure the test suite passes.
5. Make sure your code follows the existing style.
6. Issue that pull request!

## Any contributions you make will be under the MIT Software License

In short, when you submit code changes, your submissions are understood to be under the same [MIT License](http://choosealicense.com/licenses/mit/) that covers the project. Feel free to contact the maintainers if that's a concern.

## Report bugs using GitHub's [issue tracker](https://github.com/slonana-labs/limcode/issues)

We use GitHub issues to track public bugs. Report a bug by [opening a new issue](https://github.com/slonana-labs/limcode/issues/new); it's that easy!

## Write bug reports with detail, background, and sample code

**Great Bug Reports** tend to have:

- A quick summary and/or background
- Steps to reproduce
  - Be specific!
  - Give sample code if you can
- What you expected would happen
- What actually happens
- Notes (possibly including why you think this might be happening, or stuff you tried that didn't work)

## Development Process

### Building

```bash
# C++ library
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Rust crate
cargo build --release
```

### Running Tests

```bash
# C++ tests
cd build
make test

# Rust tests
cargo test --release
```

### Benchmarks

```bash
# C++ benchmarks
./build/table_bench_ultimate

# Rust benchmarks
cargo bench
```

## Code Style

### C++

- Follow Google C++ Style Guide
- Use modern C++20 features
- Prefer RAII and smart pointers
- Document complex optimizations with comments

### Rust

- Follow standard Rust formatting (`rustfmt`)
- Run `cargo clippy` before submitting
- Add doc comments for public APIs
- Include examples in documentation

## Performance Optimization Guidelines

When contributing performance optimizations:

1. **Benchmark first** - Profile before optimizing
2. **Measure impact** - Quantify the improvement
3. **Document trade-offs** - Explain what you're sacrificing (if anything)
4. **Test correctness** - Ensure optimizations don't break functionality
5. **Cross-platform** - Verify optimizations work on different CPUs

### Benchmark Requirements

All performance claims must be:
- Reproducible (include benchmark code)
- Statistically significant (1000+ iterations)
- Hardware-documented (CPU model, memory speed)
- Compared to baseline (show relative improvement)

## Adding New SIMD Optimizations

If adding SIMD intrinsics:

1. Provide fallback for CPUs without the instruction set
2. Use runtime detection where appropriate
3. Document CPU requirements in README
4. Benchmark on both Intel and AMD

## License

By contributing, you agree that your contributions will be licensed under its MIT License.

## References

This document was adapted from the open-source contribution guidelines for [Facebook's Draft](https://github.com/facebook/draft-js/blob/main/CONTRIBUTING.md).

## Questions?

Feel free to open an issue or reach out to the maintainers.

## Code of Conduct

Please note that this project is released with a [Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project you agree to abide by its terms.
