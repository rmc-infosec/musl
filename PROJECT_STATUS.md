# Project status: OSS-Fuzz readiness work (musl)

Last updated: 2025-12-21

This file summarizes the current state of the OSS-Fuzz integration prep work
done in this repo, what was learned during local fuzzing, and suggested next
steps for upstreaming and OSS-Fuzz onboarding.

## Current state

- Branch: `oss-fuzz`
- Fork / draft PR (work-in-progress): `rmc-infosec/musl` PR `#1`
- Key directories:
  - `fuzz/`: libFuzzer harnesses, seed corpora, build script, docs
  - `fuzz/oss_fuzz_build.sh`: OSS-Fuzz-compatible build entrypoint (repo-side)

## What’s implemented

### Fuzzing targets (in-tree)

Targets live under `fuzz/` and are intended to be suitable for OSS-Fuzz.

- `musl_regex_fuzzer`: `regcomp`/`regexec`
- `musl_inet_fuzzer`: `inet_pton`/`inet_ntop`
- `musl_dn_expand_fuzzer`: `dn_expand`/`dn_skipname` (and `dn_comp` exercised)
- `musl_dns_parse_fuzzer`: `__dns_parse` + `dn_expand`
- `musl_ns_parse_fuzzer`: `ns_initparse`/`ns_parserr`

Supporting artifacts:
- Per-target `.dict` and `.options`
- `fuzz/corpus/*` seed corpora; DNS seeds are generated at build time via
  `fuzz/gen_dns_seed_corpus.py`.

### OSS-Fuzz build integration strategy

This repo intentionally does **not** carry `google/oss-fuzz` repository-side
files (e.g. `project.yaml`, `Dockerfile`, etc.). Those are expected to be
submitted separately to `google/oss-fuzz`.

Repo-side build entrypoint:
- `fuzz/oss_fuzz_build.sh` builds a minimal subset of musl sources needed by the
  fuzzers, without attempting a full libc build.

Notes:
- Architecture selection is based on OSS-Fuzz `ARCHITECTURE` (or `MUSL_ARCH`),
  falling back to `uname -m`.

### Input format decision (regex fuzzer)

`musl_regex_fuzzer` expects:

- `pattern "\n" text ["\n" flags]`
- If a flags section is present, its first byte is a direct bitmask mapping to
  `REG_*` flags for easier regression input creation.

## Coverage (local snapshot)

Local coverage workflow is documented in `fuzz/COVERAGE.md`.

Snapshot (seed corpus + `-max_total_time=2` per target) was:
- **69.34% line coverage** over the **compiled subset** (not full musl).

Interpretation:
- This is an internal “how much of the built objects are being exercised” metric
  and should not be presented as whole-project musl coverage.

## Findings from local fuzzing

Documented in `fuzz/FINDINGS.md` with base64 repro blobs and command lines.

Observed issues were primarily **DoS-style** (timeouts / memory blowups) in the
regex engine.

### Fixes added (hardening)

To make these issues non-exploitable as “infinite” fuzz hangs / giant memory
allocations in the fuzzing context, a hardening patch was added:

- `src/regex/regcomp.c`: bounds AST expansion and transition-table size, and adds
  overflow checks to avoid pathological compilation-time blowups.
- `src/regex/regexec.c`: adds a backtracking “step budget” to bound worst-case
  runtime on patterns with backreferences.

These changes cause pathological inputs to fail fast with `REG_ESPACE` instead
of timing out or allocating multi-GB buffers.

## What we could not validate here

- `/tmp/ossfuzz.txt` and `/tmp/oss-fuzz-best-practices.md` were referenced during
  the session but were not present in this environment at the time of review, so
  no checklist-style “ideal submission” diff was performed against those docs.

## Recommended next steps

### For upstream (musl)

- Decide whether upstream wants:
  - The fuzz harnesses as-is, or
  - A smaller initial set (e.g. start with DNS parsing only), or
  - The harnesses but without DoS hardening (some upstreams prefer “pure” engine
    behavior and rely on OSS-Fuzz timeouts instead).
- If upstream is amenable, split commits into review-friendly pieces:
  - Harnesses/build script/docs
  - Findings documentation
  - Regex hardening (separate, because it changes behavior under extreme cases)
- Consider whether the regex hardening limits should be:
  - Compile-time only (as implemented), or
  - Configurable/tunable, or
  - Scoped to fuzzing builds only (currently they are unconditional).

### For OSS-Fuzz onboarding (separate PR to `google/oss-fuzz`)

- Create `projects/musl/` with:
  - `project.yaml`
  - `Dockerfile`
  - `build.sh` that invokes `./fuzz/oss_fuzz_build.sh`
- Decide on corpus strategy:
  - Keep minimal curated seeds in-tree (current approach)
  - Let OSS-Fuzz manage the evolving corpus (standard)
  - Optionally host additional corpora in a separate repo if needed

### For fuzz quality

- Add more targeted seeds (especially for `regexec` backtracking patterns) and
  expand dictionaries if helpful.
- Run longer local campaigns (hours-days) with ASan/UBSan and optionally MSan,
  and track any new crashes/timeouts in `fuzz/FINDINGS.md`.
- Re-run coverage periodically and record snapshots/deltas in `fuzz/COVERAGE.md`
  (or add a small `make cov` helper if desired).

