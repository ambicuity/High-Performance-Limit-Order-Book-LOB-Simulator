# Contributing to High-Performance LOB Simulator

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/High-Performance-Limit-Order-Book-LOB-Simulator.git`
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Run tests: `cd build && ctest`
6. Commit your changes: `git commit -am 'Add some feature'`
7. Push to the branch: `git push origin feature/your-feature-name`
8. Create a Pull Request

## Development Setup

### Prerequisites

- C++20 compatible compiler
- CMake 3.15+
- Python 3.8+
- pybind11
- GoogleTest (auto-fetched)

### Building for Development

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
```

### Running Tests

```bash
# C++ tests
cd build-debug
ctest --verbose

# Python tests
cd ../tests/python
python test_api.py
```

## Code Style

### C++ Style Guidelines

- Follow modern C++20 practices
- Use `clang-format` with Google style
- Prefer `constexpr` and `noexcept` where appropriate
- Use `[[nodiscard]]` for functions returning important values
- Avoid exceptions in hot paths
- Document complex algorithms with comments

Example:

```cpp
// Good
[[nodiscard]] constexpr bool is_valid() const noexcept {
    return value_ > 0;
}

// Avoid
bool isValid() {
    if (value_ > 0) {
        return true;
    }
    return false;
}
```

### Python Style Guidelines

- Follow PEP 8
- Use `black` for formatting
- Type hints encouraged
- Docstrings for public APIs

Example:

```python
def calculate_mid_price(bid: float, ask: float) -> float:
    """Calculate mid price between bid and ask.
    
    Args:
        bid: Best bid price
        ask: Best ask price
        
    Returns:
        Mid price as average of bid and ask
    """
    return (bid + ask) / 2.0
```

## Testing Requirements

All contributions must include tests:

### For C++ Changes

1. Add unit tests in `tests/cpp/`
2. Follow existing test patterns
3. Test edge cases (empty book, large orders, etc.)
4. Verify no memory leaks with valgrind (optional)

### For Python Changes

1. Add tests in `tests/python/test_api.py`
2. Test both success and failure cases
3. Verify determinism where applicable

### For Documentation

1. Update README.md if adding features
2. Add examples for new functionality
3. Update docstrings/comments

## Performance Considerations

When contributing performance-sensitive code:

1. **Avoid allocations on hot paths**: Use object pools
2. **Prefer contiguous data**: Better cache locality
3. **Benchmark**: Run `bench_match_engine` before and after
4. **Profile if needed**: Use perf, vtune, or similar tools

Example benchmark run:

```bash
cd build
./bench_match_engine --quick
```

## Pull Request Process

1. **Description**: Clearly describe what your PR does and why
2. **Tests**: Ensure all tests pass
3. **Benchmarks**: Run benchmarks if touching performance-critical code
4. **Documentation**: Update docs if changing public APIs
5. **Review**: Address review comments promptly
6. **Squash**: Clean up commit history before merge

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Performance improvement
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] All tests pass
- [ ] Benchmarks run (if applicable)

## Checklist
- [ ] Code follows project style
- [ ] Documentation updated
- [ ] No breaking changes (or clearly documented)
```

## Areas for Contribution

### High Priority

- Additional order types (iceberg, pegged)
- Market data feed improvements
- Performance optimizations
- More comprehensive benchmarks
- Documentation examples

### Medium Priority

- Multi-symbol support
- Order book depth snapshots
- Visualization tools
- Real market data integration

### Research Topics

- Alternative matching algorithms
- Memory layout optimizations
- SIMD acceleration opportunities
- FPGA/hardware acceleration

## Reporting Issues

When reporting bugs, please include:

1. **Environment**: OS, compiler version, CMake version
2. **Steps to reproduce**: Minimal code example
3. **Expected behavior**: What should happen
4. **Actual behavior**: What actually happens
5. **Logs/output**: Error messages, stack traces

## Code Review Guidelines

When reviewing PRs:

1. **Correctness**: Does the code do what it claims?
2. **Performance**: Any performance regressions?
3. **Style**: Follows project conventions?
4. **Tests**: Adequate test coverage?
5. **Documentation**: Clear and sufficient?

## Community

- Be respectful and constructive
- Help others learn and grow
- Share knowledge and insights
- Celebrate successes

## Questions?

- Open an issue for bugs or feature requests
- Start a discussion for design questions
- Contact maintainers for sensitive matters

## License

By contributing, you agree that your contributions will be licensed under the same MIT License that covers the project.

---

Thank you for contributing to the High-Performance LOB Simulator! 🚀
