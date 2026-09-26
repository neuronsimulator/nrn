# CI dependency archive (`ci/deps`)

Pinned external downloads used by NEURON CI. Goal: **stop relying on flaky third-party hosts** (Launchpad, GNU mirrors, SourceForge) for known-good blobs, while keeping an explicit upgrade path.

**Managed binaries live on a dedicated GitHub repo’s Releases**, not in the `nrn` git tree and **not** on NEURON product Releases:

**https://github.com/neuronsimulator/nrn-ci-deps/releases** → tag **`ci-deps-v1`** (or later `ci-deps-vN`)

Sphinx / website entry point: [docs/install/ci_deps.md](../../docs/install/ci_deps.md).

## Layout

| Path | Role |
|------|------|
| `MANIFEST.yml` | Catalog: id, file, sha256, upstream URL, consumers, `managed`, release base URL |
| `assets/` | **Not in git** (gitignored). Optional local scratch for `publish.sh` |
| `fetch.sh` | Resolve one asset (local → release → upstream) + verify sha256 |
| `check-upstream.sh` | Report when upstream no longer matches the pin |
| `publish.sh` | Upload local scratch files to **nrn-ci-deps** Releases |
| `install_mpich_noble.sh` | Ubuntu 24.04 wheel-test helper (first consumer) |

## `managed: true|false`

| Value | Meaning |
|-------|---------|
| **`true`** | We host the pin on **nrn-ci-deps** Releases. CI should use `fetch.sh` / install helpers. |
| **`false`** | Catalog only: known URL/sha and consumers; not yet switched off upstream. |

`managed` answers “do **we** manage this pin’s content?” — not “is this from the original software vendor.”

## Fetch order

```text
1. local:   ci/deps/assets/<file>     # optional gitignored scratch
2. release: $NRN_CI_DEPS_BASE_URL/…   # or MANIFEST default_release_base_url (nrn-ci-deps)
3. upstream: upstream_url             # last resort
```

Override with `NRN_CI_DEPS_SOURCE=local|release|upstream`.

Wheel install helpers default to **`release`** so a missing Release asset fails loudly instead of silently re-hitting Launchpad.

## Adding / promoting the next blocker

1. Reproduce the failing URL; download once; `sha256sum` it.
2. Add or update a row in `MANIFEST.yml` (`sha256`, `upstream_url`, consumers).
3. Stage files only on disk (not for commit):

   ```bash
   mkdir -p ci/deps/assets
   # copy or: NRN_CI_DEPS_SOURCE=upstream ci/deps/fetch.sh <id> ci/deps/assets
   ci/deps/publish.sh --tag ci-deps-v1   # uploads to neuronsimulator/nrn-ci-deps
   rm -f ci/deps/assets/*                # optional
   ```

4. Set `managed: true`. Ensure `default_release_base_url` matches the release tag/repo.
5. Point the pipeline at `ci/deps/fetch.sh <id>` or an install helper.
6. Open a PR on **nrn** with **text only** (MANIFEST / scripts / docs).

## Upgrading a pin

```bash
ci/deps/check-upstream.sh mpich-noble-4.2.0-5.1

# If STALE and you intend to move:
#   1. download new upstream artifacts into a temp dir / assets/
#   2. update sha256 (and file names if needed) in MANIFEST.yml
#   3. publish.sh --tag ci-deps-v1 (or a new tag + update default_release_base_url)
#   4. re-run check-upstream.sh
#   5. PR the MANIFEST (and consumer) changes only
```

There is no automatic upgrade. `check-upstream.sh` is awareness only.

## Currently managed assets (`managed: true`)

Hosted on **nrn-ci-deps** release **`ci-deps-v1`**:

- `mpich_4.2.0-5.1_{amd64,arm64}.deb` + `libmpich12_4.2.0-5.1_{amd64,arm64}.deb` — Ubuntu 24.04 wheel tests (LP#2072338); `install_mpich_noble.sh` selects by `dpkg --print-architecture`
- `ncurses-6.4.tar.gz`, `readline-8.3.tar.gz` — Mac static readline (`build_static_readline_osx.bash`)
- `readline-7.0.tar.gz`, `ncurses-6.4.tar.gz` — manylinux wheel image (`packaging/python/Dockerfile`)
- `automake-1.16.5.tar.xz` — Ubuntu MUSIC path in `neuron-ci.yml`

## Related but not yet managed (`managed: false`)

- Azure secure files: `readline7.0-ncurses6.4.tar.gz`, `mpt_headears.2.21.tar.gz`
- python.org installers (large)
- Windows NSIS / MS-MPI installers
- apt / brew / pip (package managers — different strategy)

## MPI coverage note

Wheel testing (`packaging/python/test_wheels.sh`) runs **serial** tests with MPI packages present, then **parallel** tests under both MPICH and OpenMPI. It does **not** currently isolate “MPI not installed.” Source CI mixes `NRN_ENABLE_MPI=OFF` with default MPI-linked builds; dynamic multi-MPI is primarily a **wheel** concern.

## Requirements

- `bash`, `curl` (or `wget`), `sha256sum` / `shasum`
- `python3` (stdlib only; `ci/deps/_manifest.py` parses the restricted MANIFEST subset without PyYAML)
- `gh` only for `publish.sh` (needs write access to `neuronsimulator/nrn-ci-deps`)
