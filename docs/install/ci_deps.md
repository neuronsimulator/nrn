# CI dependency archive (`ci/deps`)

NEURON CI sometimes needs fixed third-party files (``.deb`` packages, source
tarballs, installers). Fetching those from Launchpad, GNU mirrors, SourceForge,
and similar hosts can fail intermittently and block PRs or nightlies.

The **`ci/deps`** catalog pins those downloads. **Managed binaries are stored as
assets on a GitHub Release** (not in the git tree). From the repository page:
**Releases** → tag such as **`ci-deps-v1`**.

## Operator manual

Full workflow and upgrade steps:

- [ci/deps/README.md](../../ci/deps/README.md)
- [ci/deps/MANIFEST.yml](../../ci/deps/MANIFEST.yml) — catalog (text only in git)
- [Release `ci-deps-v1`](https://github.com/neuronsimulator/nrn/releases/tag/ci-deps-v1)

## Concepts

| Field | Meaning |
|-------|---------|
| **`managed: true`** | We host the pin on our GitHub Release. CI uses `fetch.sh` / install helpers. |
| **`managed: false`** | Catalog only; not yet switched off upstream. |

`managed` means **we manage the pin**, not “from the original software vendor.”

## Common commands

Run from the repository root:

```bash
# List catalog entries
ci/deps/fetch.sh --list

# Download a managed asset from the GitHub Release (default for install helpers)
NRN_CI_DEPS_SOURCE=release ci/deps/fetch.sh mpich-noble-4.2.0-5.1 /tmp/out

# Ubuntu 24.04 wheel tests: install pinned MPICH from the Release
ci/deps/install_mpich_noble.sh

# Compare pins to upstream (awareness only; does not auto-upgrade)
ci/deps/check-upstream.sh mpich-noble-4.2.0-5.1
```

Promote a new pin (files stay local/transient; then upload):

```bash
mkdir -p ci/deps/assets
# place files matching MANIFEST file: names, or fetch with SOURCE=upstream
ci/deps/publish.sh --tag ci-deps-v1
rm -f ci/deps/assets/*   # optional; assets/ is gitignored
```

## What is managed today

On release **`ci-deps-v1`**:

- Ubuntu 24.04 MPICH debs for wheel tests (LP#2072338)
- GNU **ncurses** / **readline** sources for Mac static readline and the manylinux Dockerfile
- **automake** 1.16.5 for the Ubuntu MUSIC job in `neuron-ci`

Other MANIFEST rows stay `managed: false` until promoted the same way.

## Adding the next download blocker

1. Download the failing URL once; record `sha256`.
2. Update `MANIFEST.yml`; set `managed: true` when the Release asset exists.
3. `publish.sh` to the `ci-deps-vN` release (or a new tag + update `default_release_base_url`).
4. Wire the pipeline to `fetch.sh` / an install helper.
5. PR **text only** (MANIFEST, scripts, docs) — do not commit large blobs under `ci/deps/assets/`.

Do **not** auto-upgrade pins: run `check-upstream.sh` when you want to know if
upstream changed, then bump deliberately.

## Related docs

- [Building Python Wheels](python_wheels.md) — Azure/GitHub wheel build and test
- [Windows install](windows.md) — Windows dependency download scripts
