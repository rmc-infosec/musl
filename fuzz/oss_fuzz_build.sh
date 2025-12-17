#!/usr/bin/env bash
set -euxo pipefail

shopt -s nullglob

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

: "${CC:=clang}"
: "${CFLAGS:=-O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer-no-link}"
: "${OUT:=$ROOT/out}"
: "${WORK:=$ROOT/.oss-fuzz-work}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"

detect_arch() {
	if [ -n "${ARCHITECTURE:-}" ]; then
		case "$ARCHITECTURE" in
			x86_64 | amd64) echo x86_64 ;;
			i386) echo i386 ;;
			aarch64 | arm64) echo aarch64 ;;
			arm) echo arm ;;
			*) echo "unsupported ARCHITECTURE: $ARCHITECTURE" >&2; return 1 ;;
		esac
		return 0
	fi

	local machine
	machine="$(uname -m)"
	case "$machine" in
		x86_64) echo x86_64 ;;
		i386 | i486 | i586 | i686) echo i386 ;;
		aarch64) echo aarch64 ;;
		armv6l | armv7l | armv8l | arm) echo arm ;;
		riscv64) echo riscv64 ;;
		riscv32) echo riscv32 ;;
		s390x) echo s390x ;;
		ppc64le | ppc64) echo powerpc64 ;;
		ppc) echo powerpc ;;
		loongarch64) echo loongarch64 ;;
		mips64*) echo mips64 ;;
		mips*) echo mips ;;
		*) echo "unsupported (uname -m: $machine)" >&2; return 1 ;;
	esac
	}

if [ -n "${MUSL_ARCH:-}" ]; then
	: "${MUSL_ARCH:?}"
else
	MUSL_ARCH="$(detect_arch)"
fi

OBJROOT="$WORK/musl-ossfuzz-obj"
OBJINC="$OBJROOT/include"
mkdir -p "$OUT" "$OBJINC/bits"

sed -f "$ROOT/tools/mkalltypes.sed" \
	"$ROOT/arch/$MUSL_ARCH/bits/alltypes.h.in" \
	"$ROOT/include/alltypes.h.in" \
	>"$OBJINC/bits/alltypes.h"

cp "$ROOT/arch/$MUSL_ARCH/bits/syscall.h.in" "$OBJINC/bits/syscall.h"
sed -n -e 's/__NR_/SYS_/p' <"$ROOT/arch/$MUSL_ARCH/bits/syscall.h.in" >>"$OBJINC/bits/syscall.h"

MUSL_CPPFLAGS=(
	-I"$ROOT/fuzz/include"
	-I"$ROOT/arch/$MUSL_ARCH"
	-I"$ROOT/arch/generic"
	-I"$ROOT/src/internal"
	-I"$OBJINC"
	-I"$ROOT/include"
	-D_XOPEN_SOURCE=700
	-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
	-std=c99
)

OBJDIR="$WORK/musl-ossfuzz-objs"
mkdir -p "$OBJDIR"

obj_for() {
	echo "$OBJDIR/$(echo "$1" | tr '/.' '__').o"
}

compile() {
	local src="$1"
	local obj
	obj="$(obj_for "$src")"
	"$CC" $CFLAGS "${MUSL_CPPFLAGS[@]}" -c "$ROOT/$src" -o "$obj"
}

link_fuzzer() {
	local name="$1"
	shift
	local objs=()
	for src in "$@"; do
		objs+=("$(obj_for "$src")")
	done
	"$CC" $CFLAGS -o "$OUT/$name" "${objs[@]}" $LIB_FUZZING_ENGINE
}

zip_corpus() {
	local name="$1"
	shift
	local corpus_zip="$OUT/${name}_seed_corpus.zip"
	rm -f "$corpus_zip"

	local inputs=()
	for d in "$@"; do
		inputs+=("$d"/*)
	done
	zip -j -q "$corpus_zip" "${inputs[@]}"
}

install_artifacts() {
	local name="$1"
	cp "$ROOT/fuzz/$name.dict" "$OUT/"
	cp "$ROOT/fuzz/$name.options" "$OUT/"
}

# Compile shared musl source files once.
compile src/regex/regcomp.c
compile src/regex/regexec.c
compile src/regex/tre-mem.c

compile src/network/inet_pton.c
compile src/network/inet_ntop.c

compile src/network/dn_expand.c
compile src/network/dn_comp.c
compile src/network/dn_skipname.c
compile src/network/dns_parse.c
compile src/network/ns_parse.c

# Compile fuzzer entrypoints.
compile fuzz/musl_regex_fuzzer.c
compile fuzz/musl_inet_fuzzer.c
compile fuzz/musl_dn_expand_fuzzer.c
compile fuzz/musl_dns_parse_fuzzer.c
compile fuzz/musl_ns_parse_fuzzer.c

link_fuzzer musl_regex_fuzzer \
	src/regex/regcomp.c src/regex/regexec.c src/regex/tre-mem.c \
	fuzz/musl_regex_fuzzer.c

link_fuzzer musl_inet_fuzzer \
	src/network/inet_pton.c src/network/inet_ntop.c \
	fuzz/musl_inet_fuzzer.c

link_fuzzer musl_dn_expand_fuzzer \
	src/network/dn_expand.c src/network/dn_comp.c src/network/dn_skipname.c \
	fuzz/musl_dn_expand_fuzzer.c

link_fuzzer musl_dns_parse_fuzzer \
	src/network/dns_parse.c src/network/dn_expand.c src/network/dn_skipname.c \
	fuzz/musl_dns_parse_fuzzer.c

link_fuzzer musl_ns_parse_fuzzer \
	src/network/ns_parse.c src/network/dn_expand.c src/network/dn_skipname.c \
	fuzz/musl_ns_parse_fuzzer.c

# Seed corpora.
DNS_SEEDS="$WORK/musl-dns-seeds"
rm -rf "$DNS_SEEDS"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/fuzz/gen_dns_seed_corpus.py" "$DNS_SEEDS"

zip_corpus musl_regex_fuzzer "$ROOT/fuzz/corpus/musl_regex_fuzzer"
zip_corpus musl_inet_fuzzer "$ROOT/fuzz/corpus/musl_inet_fuzzer"
zip_corpus musl_dn_expand_fuzzer "$DNS_SEEDS"
zip_corpus musl_dns_parse_fuzzer "$DNS_SEEDS"
zip_corpus musl_ns_parse_fuzzer "$DNS_SEEDS"

install_artifacts musl_regex_fuzzer
install_artifacts musl_inet_fuzzer
install_artifacts musl_dn_expand_fuzzer
install_artifacts musl_dns_parse_fuzzer
install_artifacts musl_ns_parse_fuzzer
