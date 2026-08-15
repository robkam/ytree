#!/usr/bin/env python3
"""Compile a gettext .po catalog into a binary .mo file."""

from __future__ import annotations

import argparse
import ast
import struct
from pathlib import Path


class PoCompileError(ValueError):
    pass


class CatalogEntry:
    def __init__(self, msgid: str, msgstr: str, msgctxt: str | None = None) -> None:
        self.msgid = msgid
        self.msgstr = msgstr
        self.msgctxt = msgctxt

    @property
    def key(self) -> str:
        return f"{self.msgctxt}\x04{self.msgid}" if self.msgctxt else self.msgid


def _decode_po_string(token: str) -> str:
    try:
        return ast.literal_eval(token)
    except (SyntaxError, ValueError) as exc:
        raise PoCompileError(f"invalid PO string literal: {token!r}") from exc


def parse_po(path: Path) -> list[CatalogEntry]:
    entries: list[CatalogEntry] = []
    current: dict[str, str | None] = {"msgctxt": None, "msgid": None, "msgstr": None}
    active_key: str | None = None

    def flush() -> None:
        nonlocal current, active_key
        if current["msgid"] is None:
            current = {"msgctxt": None, "msgid": None, "msgstr": None}
            active_key = None
            return
        if current["msgstr"] is None:
            raise PoCompileError(f"entry for {current['msgid']!r} is missing msgstr")
        entries.append(
            CatalogEntry(
                msgid=str(current["msgid"]),
                msgstr=str(current["msgstr"]),
                msgctxt=(str(current["msgctxt"]) if current["msgctxt"] is not None else None),
            )
        )
        current = {"msgctxt": None, "msgid": None, "msgstr": None}
        active_key = None

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line:
            flush()
            continue
        if line.startswith("#"):
            continue
        if line.startswith("msgctxt "):
            current["msgctxt"] = _decode_po_string(line[len("msgctxt "):])
            active_key = "msgctxt"
            continue
        if line.startswith("msgid "):
            if current["msgid"] is not None and current["msgstr"] is not None:
                flush()
            current["msgid"] = _decode_po_string(line[len("msgid "):])
            active_key = "msgid"
            continue
        if line.startswith("msgstr "):
            current["msgstr"] = _decode_po_string(line[len("msgstr "):])
            active_key = "msgstr"
            continue
        if line.startswith('"'):
            if active_key is None:
                raise PoCompileError(f"orphan string continuation in {path}: {raw_line!r}")
            current[active_key] = (current[active_key] or "") + _decode_po_string(line)
            continue
        if line.startswith("msgid_plural") or line.startswith("msgstr["):
            raise PoCompileError("plural catalogs are not supported by this bootstrap compiler")
        raise PoCompileError(f"unsupported PO syntax: {raw_line!r}")

    flush()
    return entries


def build_mo(entries: list[CatalogEntry]) -> bytes:
    entries = sorted(entries, key=lambda entry: entry.key)
    keys = [entry.key.encode("utf-8") for entry in entries]
    values = [entry.msgstr.encode("utf-8") for entry in entries]
    count = len(entries)
    header_size = 7 * 4
    orig_table_offset = header_size
    trans_table_offset = orig_table_offset + (count * 8)
    strings_offset = trans_table_offset + (count * 8)

    orig_table: list[tuple[int, int]] = []
    trans_table: list[tuple[int, int]] = []
    string_blob = bytearray()

    for key in keys:
        offset = strings_offset + len(string_blob)
        orig_table.append((len(key), offset))
        string_blob.extend(key)
        string_blob.append(0)
    for value in values:
        offset = strings_offset + len(string_blob)
        trans_table.append((len(value), offset))
        string_blob.extend(value)
        string_blob.append(0)

    output = bytearray()
    output.extend(struct.pack("Iiiiiii", 0x950412de, 0, count, orig_table_offset,
                              trans_table_offset, 0, 0))
    for length, offset in orig_table:
        output.extend(struct.pack("II", length, offset))
    for length, offset in trans_table:
        output.extend(struct.pack("II", length, offset))
    output.extend(string_blob)
    return bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("po_file", help="Input .po file")
    parser.add_argument("mo_file", help="Output .mo file")
    args = parser.parse_args()

    po_path = Path(args.po_file)
    mo_path = Path(args.mo_file)
    entries = parse_po(po_path)
    mo_path.parent.mkdir(parents=True, exist_ok=True)
    mo_path.write_bytes(build_mo(entries))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
