# CI dependency archive (`ci/deps`)

Pinned external downloads used by NEURON CI. Goal: **stop relying on flaky third-party hosts** (Launchpad, GNU mirrors, SourceForge) for known-good blobs, while keeping an explicit upgrade path.

**Managed binaries live on a GitHub Release**, not in the git tree. Navigate:

https://github.com/neuronsimulator/nrn/releases → tag **`ci-deps-v1`** (or later `ci-deps-vN`)

Sphinx / website entry point: [docs/install/ci_deps.md](../../docs/install/ci_deps.md).

## Layout

| Path | Role |
|------|------|
| `MANIFEST.yml` | Catalog: id, file, sha256, upstream URL, consumers, `managed`, release base URL |
| `assets/` | **Not in git** (gitignored). Optional local scratch for `publish.sh` |
| `fetch.sh` | Resolve one asset (local → release → upstream) + verify sha256 |
| `check-upstream.sh` | Report when upstream no longer matches the pin |
| `publish.sh` | Upload local scratch files to a GitHub Release |
| `install_mpich_noble.sh` | Ubuntu 24.04 wheel-test helper (first consumer) |

## `managed: true|false`

| Value | Meaning |
|-------|---------|
| **`true`** | We host the pin on our GitHub Release. CI should use `fetch.sh` / install helpers and not depend on the upstream host. |
| **`false`** | Catalog only: known URL/sha and consumers; not yet switched off upstream. |

`managed` answers “do **we** manage this pin’s content?” — not “is this from the original software vendor.”

## Fetch order

```text
1. local:   ci/deps/assets/<file>     # optional gitignored scratch
2. release: $NRN_CI_DEPS_BASE_URL/…   # or MANIFEST default_release_base_url
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
   ci/deps/publish.sh --tag ci-deps-v1   # or ci-deps-v2 for a new cut
   rm -f ci/deps/assets/*                # optional
   ```

4. Set `managed: true`. Ensure `default_release_base_url` matches the release tag.
5. Point the pipeline at `ci/deps/fetch.sh <id>` or an install helper.
6. Open a PR with **text only** (MANIFEST / scripts / docs) — no large blobs.

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

Hosted on release **`ci-deps-v1`**:

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
- `gh` only for `publish.sh`
