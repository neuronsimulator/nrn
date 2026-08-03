# CI dependency archive (`ci/deps`)

NEURON CI sometimes needs fixed third-party files (``.deb`` packages, source
tarballs, installers). Fetching those from Launchpad, GNU mirrors, SourceForge,
and similar hosts can fail intermittently and block PRs or nightlies.

The **`ci/deps`** archive pins those downloads under our control and makes
upgrades explicit.

## Operator manual

Full workflow, fetch order, and upgrade steps live next to the tools:

- [ci/deps/README.md](../../ci/deps/README.md)
- [ci/deps/MANIFEST.yml](../../ci/deps/MANIFEST.yml) — catalog of assets

## Concepts

| Field | Meaning |
|-------|---------|
| **`managed: true`** | We control the bytes (usually under `ci/deps/assets/`). CI should use `fetch.sh` / install helpers and not depend on the upstream host at build time. |
| **`managed: false`** | Catalog only: known URL/sha and consumers; not yet switched off upstream. |

`managed` means **we manage the pin**, not “from the original software vendor.”

## Common commands

Run from the repository root:

```bash
# List catalog entries
ci/deps/fetch.sh --list

# Copy a managed asset (local assets preferred)
ci/deps/fetch.sh mpich-noble-4.2.0-5.1 /tmp/out

# Ubuntu 24.04 wheel tests: install pinned MPICH (no Launchpad download)
ci/deps/install_mpich_noble.sh

# Compare pins to upstream (awareness only; does not auto-upgrade)
ci/deps/check-upstream.sh
ci/deps/check-upstream.sh mpich-noble-4.2.0-5.1
```

Optional: publish `ci/deps/assets/` to a GitHub Release with `ci/deps/publish.sh`
(see the README). Local managed files remain the default for CI.

## What is managed today

- Ubuntu 24.04 MPICH packages used by Azure and GitHub **wheel test** jobs
  (workaround for [LP#2072338](https://bugs.launchpad.net/ubuntu/+source/mpich/+bug/2072338)),
  so both OpenMPI and MPICH can exercise dynamic MPI in
  [packaging/python/test_wheels.sh](../../packaging/python/test_wheels.sh).

Other rows in the MANIFEST (readline/ncurses sources, Windows installers, etc.)
are tracked with `managed: false` until the next flaky host justifies promoting them.

## Adding the next download blocker

1. Download the failing URL once; record `sha256`.
2. Add a MANIFEST row; copy the file into `ci/deps/assets/`; set `managed: true`.
3. Point the pipeline or script at `ci/deps/fetch.sh <id>` (or a small install helper).
4. Optionally publish a release with `publish.sh`.

Do **not** auto-upgrade pins: run `check-upstream.sh` when you want to know if
upstream changed, then bump deliberately.

## Related docs

- [Building Python Wheels](python_wheels.md) — Azure/GitHub wheel build and test
- [Windows install](windows.md) — Windows dependency download scripts
