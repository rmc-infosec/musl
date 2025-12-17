# Fuzzing (OSS-Fuzz / libFuzzer)

This directory contains libFuzzer harnesses intended for integration with
Google OSS-Fuzz.

## Local quick start (clang + libFuzzer)

The build script is OSS-Fuzz compatible and can be used locally too:

```sh
mkdir -p /tmp/musl-fuzz-out
CC=clang \
CFLAGS="-O1 -g -fsanitize=address,undefined,fuzzer-no-link -fno-omit-frame-pointer" \
LIB_FUZZING_ENGINE="-fsanitize=fuzzer" \
OUT=/tmp/musl-fuzz-out \
./fuzz/oss_fuzz_build.sh
```

Then run a fuzzer, e.g.:

```sh
/tmp/musl-fuzz-out/musl_regex_fuzzer -runs=1000
```

## Harnesses

- `musl_regex_fuzzer`: POSIX regex compilation/execution (`regcomp`/`regexec`)
- `musl_inet_fuzzer`: IP address parsing/formatting (`inet_pton`/`inet_ntop`)
- `musl_dn_expand_fuzzer`: DNS name decompression (`dn_expand`/`dn_skipname`)
- `musl_dns_parse_fuzzer`: DNS answer parsing (`__dns_parse`) + name expansion
- `musl_ns_parse_fuzzer`: BIND/libresolv-style parsing (`ns_initparse`/`ns_parserr`)

## Coverage

See `fuzz/COVERAGE.md` for a local coverage workflow and an example snapshot.

## Seed corpora

The DNS-related fuzzers (`musl_dn_expand_fuzzer`, `musl_dns_parse_fuzzer`,
`musl_ns_parse_fuzzer`) use a small binary seed corpus generated at build time
by `fuzz/gen_dns_seed_corpus.py`.
