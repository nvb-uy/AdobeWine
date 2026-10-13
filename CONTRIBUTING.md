# Contributing to PhotoshopWine

Thank you for helping. PhotoshopWine is tested on one machine so far; every report from other
hardware is valuable, positive or negative.

## Testing and reporting

- **Works for you?** Open a [compatibility report](../../issues/new?template=compatibility.yml)
  with your distribution, GPU and what you tried. "Photoshop 27.10 works on Fedora 42 with
  an RX 7800 XT" is a useful report.
- **Something broke?** Run `photoshopwine report`. It asks what you were doing (for example
  "opened a 200 MB .ai file, the system froze"), collects the details with personal data
  removed (see [docs/REPORTING.md](docs/REPORTING.md)), or open a
  [bug report](../../issues/new?template=bug.yml).
- **Questions, ideas, other Adobe apps**: [Discussions](../../discussions).

Please do not post Adobe installers, Adobe binaries, licence files, access tokens or
anything copied from your Adobe account.

## Patches

1. Find the difference: run the failing scenario with Wine debug channels
   (`WINEDEBUG=+err,+seh,+loaddll` plus the relevant module, e.g. `+d2d`, `+winhttp`).
2. Write a small Windows test program for it (see `tests/`, built with
   `x86_64-w64-mingw32-gcc`). If you have Windows, run it there too: the Windows result is
   the reference.
3. Fix Wine so it gives the Windows result, without breaking Wine's own conformance tests
   for that module (`make -C <build>/dlls/<module>/tests test` when practical).
4. Add the patch as `patches/NNNN-module-short-description.patch` (`git diff` against Wine
   11.18 with all earlier patches applied), the test under `tests/`, and a line in
   `docs/PATCHES.md`.
5. Open a pull request saying what Adobe workflow it fixes and how you checked it.

`scripts/build-wine.sh` rebuilds from a clean tree with all patches. CI checks that
every patch still applies to Wine 11.18.

## Making a release

Releases are built by GitHub Actions (`.github/workflows/release.yml`):

1. Put the new version in `VERSION`, e.g. `0.3.0`.
2. In `CHANGELOG.md`, rename `## Unreleased` to `## 0.3.0 - <date>` (or move its entries
   into that section) and start a new empty `## Unreleased` above it. That section becomes
   the release notes.
3. Commit, then tag and push the tag:
   ```sh
   git tag v0.3.0 && git push origin v0.3.0
   ```
4. When the release is up, push the commit to `master` (`git push origin master`).

The workflow checks that the tag matches `VERSION`, builds Wine with `scripts/build-wine.sh`
on Ubuntu 22.04 (so the runtime runs on distributions with glibc 2.35 or newer), checks
that it starts and creates a prefix, packs it with `scripts/make-runtime-tarball.sh` and
publishes the `v0.3.0` release with `photoshopwine-runtime-0.3.0-x86_64.tar.zst` and its
`.sha256`. It then runs the distro check (`tests/distro/check.sh`) against the new release.
The first build takes about two hours; later ones are faster thanks to the compiler cache.

Nothing else is needed: once `VERSION` on `master` says `0.3.0` and the release exists, the
one-line installer and `photoshopwine update` download that runtime instead of building
it. That is why `master` is pushed last: anyone who updates while `VERSION` is ahead of the
releases builds Wine themselves, which takes hours.

To build a release for an existing tag again, delete the release on GitHub and run the
workflow from the Actions tab ("release", Run workflow, with the tag). The "draft" option
creates a draft release that the installer does not see until you publish it; publishing
it also runs the distro check.

## Upstreaming

Most PhotoshopWine patches are general Wine fixes. Getting them into WineHQ
(<https://gitlab.winehq.org/wine/wine>) helps every Wine user and shrinks this patch set.
Upstream wants Wine-style tests in `dlls/*/tests`; converting our tests to that form is a
great first contribution.

## Ground rules

- No licence circumvention, cracks, or anything that weakens Adobe's sign-in. PhotoshopWine
  must stay usable only with a legitimate subscription.
- Be kind; see [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
- By contributing you agree your contribution is licensed under LGPL-2.1-or-later.
