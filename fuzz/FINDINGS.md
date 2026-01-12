# Fuzzing findings (local)

This document captures issues found by running the `fuzz/` libFuzzer targets
locally (ASan+UBSan).

These are primarily **denial-of-service style** findings (timeouts / high memory
use) in the POSIX regex engine. They may or may not be considered actionable by
upstream, but they are useful for tracking and regression tests.

## Environment

- Repo: musl (`oss-fuzz` branch in `rmc-infosec/musl`)
- Build: `./fuzz/oss_fuzz_build.sh` with `clang` and libFuzzer
- Example output dir: `/tmp/musl-fuzz-run-out`

## Reproducing artifacts

Each finding below includes a small base64 blob. To reproduce:

```sh
OUT=/tmp/musl-fuzz-run-out

python3 - <<'PY'
import base64
blob = "BASE64_GOES_HERE"
open("/tmp/repro", "wb").write(base64.b64decode(blob))
print("wrote /tmp/repro")
PY

$OUT/musl_regex_fuzzer -runs=1 /tmp/repro
```

For timeouts, add `-timeout=N` (seconds). For memory issues, consider
`-rss_limit_mb=N` and/or `-malloc_limit_mb=N`.

## Findings

### 1) Regex compile-time timeout in `regcomp` (slow AST expansion)

- Artifact (58 bytes):
  - `musl_regex_fuzzer-timeout-04875a1d8e392cc11c14d1f1f16dbfcc44794263`
  - base64: `XmEuezIxMyx9ezAsfXsyNTAsfXsyNTMsfXswLH0KYUxiJAphLithTGIkCmEuK2J7JAphLitiezAKCg==`
- Repro:
  - `/tmp/musl-fuzz-run-out/musl_regex_fuzzer -timeout=5 -runs=1 /tmp/repro`
- Typical stack (from local run):
  - `src/regex/regcomp.c`: `tre_stack_pop_voidptr` → `tre_copy_ast` →
    `tre_expand_ast` → `regcomp`

### 2) Regex compile-time memory blow-up (OOM) during `regcomp`

- Artifact (50 bytes):
  - `regex-14-oom-154758155da627ac8c89b425b93ad110e998eae8`
  - base64: `XC4rKysrKysrKysqLiorK3szMix9ezE1Myx9YiQKYQr//////////ysrKQAAAAAAAAA=`
- Repro (fast + deterministic, forces early abort on large allocations):
  - `/tmp/musl-fuzz-run-out/musl_regex_fuzzer -runs=1 -malloc_limit_mb=1 /tmp/repro`
- Likely root-cause areas:
  - `src/regex/regcomp.c` (`tre_compute_nfl` / `tre_set_union`)
  - Large transition allocation site (seen in other inputs): `src/regex/regcomp.c:2829`
    (`transitions = xcalloc((unsigned)add + 1, sizeof(*transitions));`)

### 3) Regex match-time timeout in `regexec` (backtracking)

- Artifact (186 bytes):
  - `regex-16-timeout-cf9b975346c523f62f64f3404ead22e5b294444a`
  - base64: `XCgoKlwxKFwrcmVcfFx8ZVwpQ0NDKioqKioqXCoqXykqXyoqXEJleDQodF87X18KQVxZKCopX19fX0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQyRDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDNTUAAAb//ycK`
- Repro:
  - `/tmp/musl-fuzz-run-out/musl_regex_fuzzer -timeout=2 -runs=1 /tmp/repro`
- Typical stack (from local run):
  - `src/regex/regexec.c:882` (`tre_tnfa_run_backtrack`) → `regexec`

### 4) Slow regex compile input (may time out under tighter settings)

This input was saved as a timeout artifact during a run, but does not always
re-trigger a hard timeout when executed once (it is still slow on some runs).

- Artifact (70 bytes):
  - `regex-2-timeout-53260799f5aed4f22c0d7a406df444dc1af090bf`
  - base64: `XHRcdHQsfXswLH17Mix9ezAsfXsyNTMsfXswLH0KYUxiJAphLiticzACCmFMMgAAAABhTDIAAAAANTMsAAAAMH17CgowLA==`

