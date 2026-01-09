# Contribution Guide

All contributions to the cpp-clob-client are welcome and greatly appreciated! This document serves to outline the process for contributions and help you get set up.

## Steps to Get Started

1. Fork [Polymarket/cpp-clob-client](https://github.com/polymarket/cpp-clob-client)
2. Clone your fork
3. Install dependencies (see README.md)
4. Build the project
5. Open pull requests with the `wip` label against the `main` branch and include a description of the intended change in the PR description

Before removing the `wip` label and submitting a PR for review, make sure that:

- It passes all checks, including lints and tests
- Your fork is up to date with `main`

## Building and Testing

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
make test
```

## Branch Structure & Naming

Our main branch, `main`, represents the current development state of the codebase. All pull requests should be opened against `main`.

Please follow the [conventional commits](https://www.conventionalcommits.org/) standard when naming your PR.

Examples:
- `feat: add websocket support`
- `fix: correct order signing for neg risk markets`
- `docs: update API reference`
- `refactor: simplify order builder`
- `test: add integration tests for approvals`

## Code Style

- Use C++20 features where appropriate
- Follow existing code conventions in the project
- Use descriptive variable and function names
- Add comments for complex logic
- Keep functions focused and single-purpose

## Testing

- Add tests for new functionality
- Ensure existing tests pass
- Use descriptive test names

## Pull Request Guidelines

1. **Keep PRs focused**: One feature or fix per PR
2. **Write clear descriptions**: Explain what the PR does and why
3. **Update documentation**: If your change affects the public API, update the README
4. **Add tests**: Cover your changes with appropriate tests
5. **Respond to feedback**: Address review comments promptly

## Reporting Issues

When reporting issues, please include:

- A clear description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Your environment (OS, compiler version, etc.)
- Relevant code snippets or error messages

## Security

If you discover a security vulnerability, please do NOT open an issue. Email security@polymarket.com instead.

## Questions?

Feel free to open an issue for questions about contributing.

