# Release Process

This document describes the release process for hyprlax.

## Versioning

We follow [Semantic Versioning](https://semver.org/):
- **MAJOR** version for incompatible API changes
- **MINOR** version for backwards-compatible functionality additions  
- **PATCH** version for backwards-compatible bug fixes

Format: `v{MAJOR}.{MINOR}.{PATCH}` (e.g., `v1.0.0`, `v1.2.3`)

## Release Workflow

### 1. Prepare the Release

1. Ensure all changes are committed and pushed to `main`
2. Update version references if needed
3. Ensure CI passes on `main`

### 2. Create and Push Tag

```bash
# Create annotated tag
git tag -a v1.0.0 -m "Release v1.0.0: Initial release"

# Push tag to trigger release workflow
git push origin v1.0.0
```

### 3. Automated Release Process

When a tag is pushed, GitHub Actions will automatically:
1. Build the binary for multiple architectures
2. Run tests
3. Create a GitHub release
4. Upload built binaries as release assets
5. Generate release notes from commits

### 4. Manual Steps (if needed)

After the automated release:
1. Edit the release notes on GitHub if needed
2. Announce the release (Discord, Reddit, etc.)

## Release Checklist

- [ ] Code is tested locally
- [ ] README is up to date
- [ ] No debug code left in
- [ ] Version tag follows semantic versioning
- [ ] Tag message describes key changes

## Tag Message Format

```
Release v{VERSION}: {Brief description}

Key changes:
- Feature: {description}
- Fix: {description}
- Enhancement: {description}
```

Example:
```
Release v1.1.0: Enhanced animation system

Key changes:
- Feature: Added elastic and bounce easing functions
- Fix: Resolved edge stretching on large workspace numbers
- Enhancement: Improved animation performance by 30%
```

## Rollback Process

If a release has critical issues:

1. Delete the release on GitHub (keep the tag for history)
2. Fix the issue
3. Create a new patch version tag
4. Push the new tag

```bash
# Example: Rolling back v1.0.0 and releasing fix as v1.0.1
git tag -a v1.0.1 -m "Release v1.0.1: Critical fix for workspace detection"
git push origin v1.0.1
```

## Development Releases

For testing releases, use pre-release tags:
- `v1.0.0-alpha.1`
- `v1.0.0-beta.1`
- `v1.0.0-rc.1`

These won't be marked as "latest" on GitHub.

## Notes

- Always create tags from the `main` branch
- Never force-push to existing tags
- Keep release binaries under 50MB for easy distribution
- Test the release binary on a clean system before announcing