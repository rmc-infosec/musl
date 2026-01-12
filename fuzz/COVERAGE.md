# Local coverage

This repo contains a small set of OSS-Fuzz/libFuzzer targets that only build
selected musl source files (not the full libc). The coverage numbers below are
therefore **coverage of the compiled fuzzing targets**, not overall musl libc
coverage.

## Reproduce (example)

```sh
rm -rf /tmp/musl-fuzz-cov-out /tmp/musl-fuzz-cov-work
mkdir -p /tmp/musl-fuzz-cov-out /tmp/musl-fuzz-cov-work

CC=clang \
CFLAGS='-O1 -g -fprofile-instr-generate -fcoverage-mapping -fsanitize=fuzzer-no-link -fno-omit-frame-pointer' \
LIB_FUZZING_ENGINE='-fsanitize=fuzzer' \
OUT=/tmp/musl-fuzz-cov-out \
WORK=/tmp/musl-fuzz-cov-work \
./fuzz/oss_fuzz_build.sh

# Unzip seed corpora and run each fuzzer briefly to collect profiles.
rm -rf /tmp/musl-fuzz-cov-corpus /tmp/musl-fuzz-cov-prof
mkdir -p /tmp/musl-fuzz-cov-corpus /tmp/musl-fuzz-cov-prof
for f in musl_regex_fuzzer musl_inet_fuzzer musl_dn_expand_fuzzer musl_dns_parse_fuzzer musl_ns_parse_fuzzer; do
  mkdir -p /tmp/musl-fuzz-cov-corpus/$f
  unzip -q /tmp/musl-fuzz-cov-out/${f}_seed_corpus.zip -d /tmp/musl-fuzz-cov-corpus/$f
  LLVM_PROFILE_FILE=/tmp/musl-fuzz-cov-prof/${f}.profraw \
    /tmp/musl-fuzz-cov-out/$f -max_total_time=2 -seed=1 /tmp/musl-fuzz-cov-corpus/$f >/dev/null 2>&1 || true
done

llvm-profdata merge -sparse /tmp/musl-fuzz-cov-prof/*.profraw -o /tmp/musl-fuzz-cov-prof/merged.profdata
llvm-cov report -instr-profile=/tmp/musl-fuzz-cov-prof/merged.profdata \
  -object /tmp/musl-fuzz-cov-out/musl_regex_fuzzer \
  -object /tmp/musl-fuzz-cov-out/musl_inet_fuzzer \
  -object /tmp/musl-fuzz-cov-out/musl_dn_expand_fuzzer \
  -object /tmp/musl-fuzz-cov-out/musl_dns_parse_fuzzer \
  -object /tmp/musl-fuzz-cov-out/musl_ns_parse_fuzzer
```

## Snapshot (seed corpus + 2s fuzzing per target)

From `llvm-cov report` (clang/llvm 18.1.3):

```
include/ctype.h                        1                 1     0.00%           1                 1     0.00%           3                 3     0.00%           0                 0         -
src/network/dn_comp.c                 92                12    86.96%           4                 0   100.00%          84                 6    92.86%          68                21    69.12%
src/network/dn_expand.c               38                 1    97.37%           1                 0   100.00%          26                 0   100.00%          28                 2    92.86%
src/network/dn_skipname.c             16                 0   100.00%           1                 0   100.00%          12                 0   100.00%          10                 0   100.00%
src/network/dns_parse.c               33                 1    96.97%           1                 0   100.00%          27                 0   100.00%          24                 1    95.83%
src/network/inet_ntop.c               32                 0   100.00%           1                 0   100.00%          45                 0   100.00%          22                 0   100.00%
src/network/inet_pton.c              108                 1    99.07%           2                 0   100.00%          60                 0   100.00%          72                 0   100.00%
src/network/ns_parse.c               125                14    88.80%           8                 2    75.00%         126                24    80.95%          58                 9    84.48%
src/regex/regcomp.c                 2185               569    73.96%          38                 1    97.37%        2056               410    80.06%        1188               348    70.71%
src/regex/regexec.c                  998               564    43.49%           6                 1    83.33%         572               292    48.95%         660               390    40.91%
src/regex/tre-mem.c                   65                17    73.85%           3                 0   100.00%          81                26    67.90%          28                 8    71.43%
TOTAL                               3898              1195    69.34%          77                 5    93.51%        3292               768    76.67%        2300               815    64.57%
```
