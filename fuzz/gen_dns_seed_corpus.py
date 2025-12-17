#!/usr/bin/env python3

import os
import struct
import sys


def dns_name(name: str) -> bytes:
	parts = [p for p in name.split(".") if p]
	out = bytearray()
	for p in parts:
		# RFC 1035 label length is 63; keep seeds valid-ish.
		p_bytes = p.encode("ascii", "ignore")[:63]
		out.append(len(p_bytes))
		out += p_bytes
	out.append(0)
	return bytes(out)


def dns_header(msg_id: int, flags: int, qd: int, an: int, ns: int, ar: int) -> bytes:
	return struct.pack("!HHHHHH", msg_id & 0xFFFF, flags & 0xFFFF, qd & 0xFFFF, an & 0xFFFF, ns & 0xFFFF, ar & 0xFFFF)


def rr(name_ptr: bytes, rrtype: int, rrclass: int, ttl: int, rdata: bytes) -> bytes:
	return name_ptr + struct.pack("!HHIH", rrtype & 0xFFFF, rrclass & 0xFFFF, ttl & 0xFFFFFFFF, len(rdata) & 0xFFFF) + rdata


def msg_a_response() -> bytes:
	# Minimal response with a compressed NAME pointer in the answer.
	qname = dns_name("example.com")
	hdr = dns_header(0x1234, 0x8180, 1, 1, 0, 0)
	question = qname + struct.pack("!HH", 1, 1)  # A / IN
	answer = rr(b"\xC0\x0C", 1, 1, 0, b"\x5D\xB8\xD8\x22")  # 93.184.216.34
	return hdr + question + answer


def msg_aaaa_response() -> bytes:
	qname = dns_name("example.com")
	hdr = dns_header(0x1234, 0x8180, 1, 1, 0, 0)
	question = qname + struct.pack("!HH", 28, 1)  # AAAA / IN
	ip6 = bytes.fromhex("2606:2800:0220:0001:0248:1893:25c8:1946".replace(":", ""))
	answer = rr(b"\xC0\x0C", 28, 1, 0, ip6)
	return hdr + question + answer


def msg_cname_response() -> bytes:
	# Response where RDATA is also a (compressed) name.
	qname = dns_name("www.example.com")
	hdr = dns_header(0x1234, 0x8180, 1, 1, 0, 0)
	question = qname + struct.pack("!HH", 5, 1)  # CNAME / IN
	# Point RDATA at the "example.com" suffix within the QNAME: "www" "." "example" "." "com"
	# QNAME starts at offset 12; "example" label starts at 12 + 1 + len("www") = 16.
	answer = rr(b"\xC0\x0C", 5, 1, 0, b"\xC0\x10")
	return hdr + question + answer


def msg_self_pointer_loop() -> bytes:
	# Construct a message where offset 12 is a compression pointer back to itself.
	msg = bytearray(b"\x00" * 32)
	msg[12] = 0xC0
	msg[13] = 0x0C
	return bytes(msg)


def msg_pointer_chain_loop() -> bytes:
	# Two pointers that bounce between offsets 12 and 14.
	msg = bytearray(b"\x00" * 32)
	msg[12:14] = b"\xC0\x0E"  # -> 14
	msg[14:16] = b"\xC0\x0C"  # -> 12
	return bytes(msg)


def write(out_dir: str, name: str, data: bytes) -> None:
	path = os.path.join(out_dir, name)
	with open(path, "wb") as f:
		f.write(data)


def main(argv: list[str]) -> int:
	if len(argv) != 2:
		print(f"usage: {argv[0]} OUT_DIR", file=sys.stderr)
		return 2

	out_dir = argv[1]
	os.makedirs(out_dir, exist_ok=True)

	write(out_dir, "a_response.bin", msg_a_response())
	write(out_dir, "aaaa_response.bin", msg_aaaa_response())
	write(out_dir, "cname_response.bin", msg_cname_response())
	write(out_dir, "self_ptr_loop.bin", msg_self_pointer_loop())
	write(out_dir, "ptr_chain_loop.bin", msg_pointer_chain_loop())

	return 0


if __name__ == "__main__":
	raise SystemExit(main(sys.argv))

