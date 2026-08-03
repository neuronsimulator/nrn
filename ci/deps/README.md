# CI dependency archive (`ci/deps`)

Pinned external downloads used by NEURON CI. Goal: **stop relying on flaky third-party hosts** (Launchpad, GNU mirrors, SourceForge) for known-good blobs, while keeping an explicit upgrade path.

Sphinx / website entry point (same topic, shorter):
[docs/install/ci_deps.md](../../docs/install/ci_deps.md).

## Layout

| Path | Role |
|------|------|
| `MANIFEST.yml` | Catalog: id, file name, sha256, upstream URL, consumers, `managed` |
| `assets/` | Managed blobs (committed). Preferred fetch source |
| `fetch.sh` | Resolve one asset (local → release URL → upstream) + verify sha256 |
| `check-upstream.sh` | Report when upstream no longer matches the pin |
| `publish.sh` | Optional: upload `assets/` to a GitHub Release |
| `install_mpich_noble.sh` | Ubuntu 24.04 wheel-test helper (first consumer) |

## `managed: true|false`

| Value | Meaning |
|-------|---------|
| **`true`** | We control the bytes (`assets/` and/or a release we publish). CI should use `fetch.sh` / install helpers and not depend on the upstream host. |
| **`false`** | Catalog only: known URL/sha and consumers; not yet switched off upstream (or not yet copied into `assets/`). |

`managed` answers “do **we** manage this pin’s content?” — not “is this from the original software vendor.”

## Fetch order

```text
1. local:   ci/deps/assets/<file>
2. release: $NRN_CI_DEPS_BASE_URL/<file>   # optional GitHub Release
3. upstream: upstream_url from MANIFEST    # last resort
```

Override with `NRN_CI_DEPS_SOURCE=local|release|upstream`.

Wheel jobs force **`local`** via `install_mpich_noble.sh` so a missing managed file fails loudly instead of re-hitting Launchpad.

## Adding the next blocker (incremental pattern)

1. Reproduce the failing URL; download once; `sha256sum` it.
2. Add a row to `MANIFEST.yml` with `managed: false` (or `true` if adding the blob in the same PR).
3. Copy the file into `assets/` and set `managed: true`.
4. Point the pipeline/script at `ci/deps/fetch.sh <id>`.
5. Optionally `publish.sh` so clones can also use a Release URL.

## Upgrading a pin

Self-hosting **is** pinning. Upgrades are intentional:

```bash
# Does upstream still match our sha256?
ci/deps/check-upstream.sh mpich-noble-4.2.0-5.1

# If STALE and you intend to move:
#   1. download new upstream artifacts
#   2. replace assets/ and update sha256 (and file names if needed)
#   3. update install helpers / consumers
#   4. re-run check-upstream.sh (should OK)
#   5. optional: publish.sh --tag ci-deps-v2
```

There is no automatic upgrade. `check-upstream.sh` is awareness only.

## Currently managed assets (`managed: true`)

- `mpich_4.2.0-5.1_amd64.deb` + `libmpich12_4.2.0-5.1_amd64.deb`  
  Ubuntu 24.04 workaround for [LP#2072338](https://bugs.launchpad.net/ubuntu/+source/mpich/+bug/2072338), used by Azure and GitHub wheel tests so both OpenMPI and MPICH can exercise dynamic MPI.

## Related but not yet managed (`managed: false`)

- Azure secure files: `readline7.0-ncurses6.4.tar.gz`, `mpt_headears.2.21.tar.gz`
- GNU sources for Mac/Linux static readline
- python.org installers (large)
- apt / brew / pip (package managers — different strategy)

## MPI coverage note

Wheel testing (`packaging/python/test_wheels.sh`) runs **serial** tests with MPI packages present, then **parallel** tests under both MPICH and OpenMPI. It does **not** currently isolate “MPI not installed.” Source CI mixes `NRN_ENABLE_MPI=OFF` with default MPI-linked builds; dynamic multi-MPI is primarily a **wheel** concern.

## Requirements

- `bash`, `curl` (or `wget`), `sha256sum` / `shasum`
- `python3` (stdlib only; `ci/deps/_manifest.py` parses the restricted MANIFEST subset without PyYAML)
