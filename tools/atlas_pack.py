from __future__ import annotations

import argparse
import hashlib
import re
import secrets
import struct
import subprocess
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


ATLX_MAGIC = 0x584C5441
ATLX_VERSION = 3
ATLX_HEADER_SIZE = 112
ATLX_FLAG_ENTRY_ATLAS_START = 1 << 0
ATLX_IMPORT_FLAG_ORDINAL = 1 << 0

IMAGE_DOS_SIGNATURE = 0x5A4D
IMAGE_NT_SIGNATURE = 0x00004550
IMAGE_FILE_MACHINE_AMD64 = 0x8664
IMAGE_FILE_MACHINE_I386 = 0x014C
IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10B
IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20B
IMAGE_DIRECTORY_ENTRY_EXPORT = 0
IMAGE_DIRECTORY_ENTRY_IMPORT = 1
IMAGE_DIRECTORY_ENTRY_BASERELOC = 5
IMAGE_DIRECTORY_ENTRY_TLS = 9
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_MEM_WRITE = 0x80000000
IMAGE_REL_BASED_ABSOLUTE = 0
IMAGE_REL_BASED_HIGHLOW = 3
IMAGE_REL_BASED_DIR64 = 10
IMAGE_ORDINAL_FLAG32 = 0x80000000
IMAGE_ORDINAL_FLAG64 = 0x8000000000000000
ATLAS_HOOK_SECTION = b".atlshk"
ATLAS_HOOK_RECORD_SIZE = 16
LSTR_SECTION_NAME = b".lstr"
LSTR_COOKIE_SIZE = 16
LSTR_MARKER_PLAINTEXT = b"ATLPLAIN"
LSTR_MARKER_ENCRYPTED = b"ATLXORED"

FNV64_OFFSET = 14695981039346656037
FNV64_PRIME = 1099511628211
FNV32_OFFSET = 2166136261
FNV32_PRIME = 16777619


@dataclass(frozen=True)
class Section:
    name: bytes
    virtual_size: int
    virtual_address: int
    raw_size: int
    raw_pointer: int
    characteristics: int

    @property
    def mapped_size(self) -> int:
        return self.virtual_size or self.raw_size


@dataclass(frozen=True)
class PELayout:
    data: bytes
    machine: int
    image_base: int
    image_size: int
    headers_size: int
    entry_rva: int
    directories: list[tuple[int, int]]
    sections: list[Section]


def require(value: bool, message: str) -> None:
    if not value:
        raise ValueError(message)


def range_valid(total: int, offset: int, size: int) -> bool:
    return offset >= 0 and size >= 0 and offset <= total and size <= total - offset


def u16(data: bytes, offset: int) -> int:
    require(range_valid(len(data), offset, 2), "short read")
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    require(range_valid(len(data), offset, 4), "short read")
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    require(range_valid(len(data), offset, 8), "short read")
    return struct.unpack_from("<Q", data, offset)[0]


def c_string_at(data: bytes, offset: int, limit: int = 4096) -> bytes:
    require(range_valid(len(data), offset, 1), "string outside file")
    end = offset
    maximum = min(len(data), offset + limit)
    while end < maximum and data[end] != 0:
        end += 1
    require(end < maximum, "unterminated string")
    return data[offset:end]


def fnv1a64(value: bytes, lower_ascii: bool = False) -> int:
    result = FNV64_OFFSET
    for byte in value:
        if lower_ascii and 0x41 <= byte <= 0x5A:
            byte += 0x20
        result ^= byte
        result = (result * FNV64_PRIME) & 0xFFFFFFFFFFFFFFFF
    return result


def fnv1a32(value: bytes) -> int:
    result = FNV32_OFFSET
    for byte in value:
        result ^= byte
        result = (result * FNV32_PRIME) & 0xFFFFFFFF
    return result


def parse_pe(data: bytes) -> PELayout:
    require(len(data) >= 0x1000, "input is too small")
    require(u16(data, 0) == IMAGE_DOS_SIGNATURE, "missing DOS signature")
    e_lfanew = u32(data, 0x3C)
    require(e_lfanew > 0 and range_valid(len(data), e_lfanew, 24), "bad NT header offset")
    require(u32(data, e_lfanew) == IMAGE_NT_SIGNATURE, "missing NT signature")

    file_header = e_lfanew + 4
    machine, section_count, _, _, _, optional_size, _ = struct.unpack_from("<HHIIIHH", data, file_header)
    require(machine in (IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386),
            "only AMD64 and I386 PE images are supported")
    require(0 < section_count <= 96, "invalid section count")

    optional_offset = file_header + 20
    is_pe64 = machine == IMAGE_FILE_MACHINE_AMD64
    expected_magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC if is_pe64 else IMAGE_NT_OPTIONAL_HDR32_MAGIC
    minimum_optional = 112 if is_pe64 else 96
    require(optional_size >= minimum_optional, "optional header is too small")
    require(range_valid(len(data), optional_offset, optional_size), "optional header outside file")
    require(u16(data, optional_offset) == expected_magic, "optional header magic does not match machine")

    entry_rva = u32(data, optional_offset + 16)
    if is_pe64:
        image_base = u64(data, optional_offset + 24)
        section_alignment = u32(data, optional_offset + 32)
        file_alignment = u32(data, optional_offset + 36)
        image_size = u32(data, optional_offset + 56)
        headers_size = u32(data, optional_offset + 60)
        directory_count = u32(data, optional_offset + 108)
        directory_base = optional_offset + 112
    else:
        image_base = u32(data, optional_offset + 28)
        section_alignment = u32(data, optional_offset + 32)
        file_alignment = u32(data, optional_offset + 36)
        image_size = u32(data, optional_offset + 56)
        headers_size = u32(data, optional_offset + 60)
        directory_count = u32(data, optional_offset + 92)
        directory_base = optional_offset + 96

    require(section_alignment >= 0x1000 and section_alignment & (section_alignment - 1) == 0, "bad section alignment")
    require(file_alignment != 0 and file_alignment & (file_alignment - 1) == 0, "bad file alignment")
    require(image_size >= 0x1000 and image_size <= 32 * 1024 * 1024, "bad image size")
    require(headers_size != 0 and headers_size <= len(data), "bad header size")
    require(entry_rva < image_size, "bad entry rva")

    directories: list[tuple[int, int]] = []
    for index in range(16):
        if index >= directory_count:
            directories.append((0, 0))
            continue
        directory_offset = directory_base + index * 8
        require(directory_offset + 8 <= optional_offset + optional_size, "data directory outside optional header")
        directories.append((u32(data, directory_offset), u32(data, directory_offset + 4)))

    sections: list[Section] = []
    section_offset = optional_offset + optional_size
    require(range_valid(len(data), section_offset, section_count * 40), "section table outside file")
    for index in range(section_count):
        current = section_offset + index * 40
        name = data[current:current + 8].split(b"\0", 1)[0]
        virtual_size = u32(data, current + 8)
        virtual_address = u32(data, current + 12)
        raw_size = u32(data, current + 16)
        raw_pointer = u32(data, current + 20)
        characteristics = u32(data, current + 36)
        mapped_size = virtual_size or raw_size
        require(mapped_size != 0, "zero-size section")
        require(virtual_address % section_alignment == 0, "misaligned section")
        require(range_valid(image_size, virtual_address, mapped_size), "section outside image")
        if raw_size != 0:
            require(raw_pointer != 0, "raw section with null file pointer")
            require(range_valid(len(data), raw_pointer, raw_size), "raw section outside file")
        sections.append(Section(name, virtual_size, virtual_address, raw_size, raw_pointer, characteristics))

    require(all(section.name != b".CRT" for section in sections), "CRT initializers are not supported by ATLX v1")

    tls_rva, tls_size = directories[IMAGE_DIRECTORY_ENTRY_TLS]
    require(tls_rva == 0 and tls_size == 0, "TLS callbacks are not supported by ATLX v1")
    return PELayout(data, machine, image_base, image_size, headers_size, entry_rva, directories, sections)


def rva_to_file(layout: PELayout, rva: int, size: int) -> int:
    require(size > 0, "zero-size RVA read")
    if rva < layout.headers_size and range_valid(len(layout.data), rva, size):
        return rva
    for section in layout.sections:
        if section.raw_size == 0 or rva < section.virtual_address:
            continue
        delta = rva - section.virtual_address
        if delta <= section.raw_size and size <= section.raw_size - delta:
            offset = section.raw_pointer + delta
            require(range_valid(len(layout.data), offset, size), "RVA maps outside file")
            return offset
    raise ValueError(f"RVA 0x{rva:x} outside raw image")


def rva_c_string(layout: PELayout, rva: int, limit: int = 4096) -> bytes:
    offset = rva_to_file(layout, rva, 1)
    return c_string_at(layout.data, offset, limit)


def rva_is_executable(layout: PELayout, rva: int) -> bool:
    for section in layout.sections:
        if rva < section.virtual_address:
            continue
        if rva - section.virtual_address >= section.mapped_size:
            continue
        return (section.characteristics & IMAGE_SCN_MEM_EXECUTE) != 0
    return False


def resolve_export_rva(layout: PELayout, name: bytes) -> int:
    export_rva, export_size = layout.directories[IMAGE_DIRECTORY_ENTRY_EXPORT]
    require(export_rva != 0 and export_size >= 40, "missing export directory")
    export_offset = rva_to_file(layout, export_rva, 40)
    fields = struct.unpack_from("<IIHHIIIIIII", layout.data, export_offset)
    _, _, _, _, _, ordinal_base, function_count, name_count, functions_rva, names_rva, ordinals_rva = fields
    require(function_count > 0 and name_count > 0, "empty export directory")
    functions_offset = rva_to_file(layout, functions_rva, function_count * 4)
    names_offset = rva_to_file(layout, names_rva, name_count * 4)
    ordinals_offset = rva_to_file(layout, ordinals_rva, name_count * 2)

    for index in range(name_count):
        name_rva = u32(layout.data, names_offset + index * 4)
        exported = rva_c_string(layout, name_rva, 256)
        if exported != name:
            continue
        ordinal = u16(layout.data, ordinals_offset + index * 2)
        require(ordinal < function_count, "export ordinal outside table")
        entry_rva = u32(layout.data, functions_offset + ordinal * 4)
        require(entry_rva != 0 and entry_rva < layout.image_size, "export RVA outside image")
        require(not (export_rva <= entry_rva < export_rva + export_size), "forwarded entry export is not supported")
        require(rva_is_executable(layout, entry_rva), "entry export is not executable")
        _ = ordinal_base
        return entry_rva

    raise ValueError(f"missing export {name.decode('ascii', errors='replace')}")


def collect_relocations(layout: PELayout) -> list[int]:
    relocation_rva, relocation_size = layout.directories[IMAGE_DIRECTORY_ENTRY_BASERELOC]
    if relocation_rva == 0 or relocation_size == 0:
        return []

    is_pe64 = layout.machine == IMAGE_FILE_MACHINE_AMD64
    expected_type = IMAGE_REL_BASED_DIR64 if is_pe64 else IMAGE_REL_BASED_HIGHLOW
    pointer_size = 8 if is_pe64 else 4

    relocation_offset = rva_to_file(layout, relocation_rva, relocation_size)
    relocation_end = relocation_offset + relocation_size
    records: list[int] = []
    cursor = relocation_offset
    while cursor < relocation_end:
        require(cursor + 8 <= relocation_end, "truncated relocation block")
        page_rva = u32(layout.data, cursor)
        block_size = u32(layout.data, cursor + 4)
        require(block_size >= 8 and cursor + block_size <= relocation_end, "bad relocation block")
        require(range_valid(layout.image_size, page_rva, block_size), "relocation block outside image")
        entry_count = (block_size - 8) // 2
        entries = cursor + 8
        for index in range(entry_count):
            entry = u16(layout.data, entries + index * 2)
            relocation_type = entry >> 12
            offset = entry & 0x0FFF
            if relocation_type == IMAGE_REL_BASED_ABSOLUTE:
                continue
            require(relocation_type == expected_type, "unsupported relocation type")
            target_rva = page_rva + offset
            require(range_valid(layout.image_size, target_rva, pointer_size), "relocation target outside image")
            records.append(target_rva)
        cursor += block_size

    return records


def collect_imports(layout: PELayout) -> list[tuple[int, int, int, int, int]]:
    import_rva, import_size = layout.directories[IMAGE_DIRECTORY_ENTRY_IMPORT]
    if import_rva == 0 or import_size == 0:
        return []

    is_pe64 = layout.machine == IMAGE_FILE_MACHINE_AMD64
    thunk_size = 8 if is_pe64 else 4
    thunk_reader = u64 if is_pe64 else u32
    ordinal_flag = IMAGE_ORDINAL_FLAG64 if is_pe64 else IMAGE_ORDINAL_FLAG32
    hint_name_mask = 0x7FFFFFFFFFFFFFFF if is_pe64 else 0x7FFFFFFF

    import_offset = rva_to_file(layout, import_rva, import_size)
    import_end = import_offset + import_size
    imports: list[tuple[int, int, int, int, int]] = []
    cursor = import_offset
    while cursor + 20 <= import_end:
        original_first_thunk, _, _, name_rva, first_thunk = struct.unpack_from("<IIIII", layout.data, cursor)
        if original_first_thunk == 0 and name_rva == 0 and first_thunk == 0:
            break
        require(name_rva != 0 and first_thunk != 0, "bad import descriptor")
        module_name = rva_c_string(layout, name_rva, 260)
        require(module_name != b"", "empty import module")
        thunk_rva = original_first_thunk or first_thunk
        thunk_offset = rva_to_file(layout, thunk_rva, thunk_size)
        index = 0
        while True:
            value = thunk_reader(layout.data, thunk_offset + index * thunk_size)
            if value == 0:
                break
            iat_rva = first_thunk + index * thunk_size
            require(range_valid(layout.image_size, iat_rva, thunk_size), "IAT RVA outside image")
            module_hash = fnv1a64(module_name, True)
            if value & ordinal_flag:
                ordinal = value & 0xFFFF
                require(ordinal != 0, "zero ordinal import")
                imports.append((module_hash, 0, iat_rva, ordinal, ATLX_IMPORT_FLAG_ORDINAL))
            else:
                import_name_rva = value & hint_name_mask
                name_offset = rva_to_file(layout, import_name_rva, 3)
                symbol_name = c_string_at(layout.data, name_offset + 2, 512)
                require(symbol_name != b"", "empty import symbol")
                imports.append((module_hash, fnv1a64(symbol_name, False), iat_rva, 0, 0))
            index += 1
            require(len(imports) <= 8192, "too many imports")
        cursor += 20

    return imports


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def encrypt_lstr(chunk: bytes, key: bytes) -> bytes:
    require(len(chunk) >= LSTR_COOKIE_SIZE, ".lstr section is smaller than its cookie")
    require(chunk[:8] == LSTR_MARKER_PLAINTEXT,
            ".lstr section does not start with the plaintext cookie "
            "(already packed, or lstr.cpp was dropped)")
    require(len(key) == 8, "key must be 8 bytes")

    out = bytearray(len(chunk))
    out[:8] = LSTR_MARKER_ENCRYPTED
    out[8:16] = key
    for index in range(LSTR_COOKIE_SIZE, len(chunk)):
        out[index] = chunk[index] ^ key[index & 7] ^ ((index * 73 + 0x9D) & 0xFF)
    return bytes(out)


def build_atlx(
    layout: PELayout,
    entry_rva: int,
    hooks: list[tuple[int, int]],
    randomize: bool,
) -> bytes:
    section_records: list[bytes] = []
    raw_chunks: list[bytes] = []
    raw_cursor = 0

    lstr_key = secrets.token_bytes(8) if randomize else bytes(range(8))
    lstr_encrypted = False

    for section in layout.sections:
        raw_size = 0
        raw_offset = 0
        characteristics = section.characteristics
        if section.raw_size != 0 and (section.characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0:
            raw_size = min(section.raw_size, section.mapped_size)
            chunk = layout.data[section.raw_pointer:section.raw_pointer + raw_size]
            if section.name == LSTR_SECTION_NAME:
                chunk = encrypt_lstr(chunk, lstr_key)
                characteristics |= IMAGE_SCN_MEM_WRITE
                lstr_encrypted = True
            raw_offset = raw_cursor
            raw_chunks.append(chunk)
            raw_cursor += raw_size
        section_records.append(struct.pack(
            "<IIIIII",
            section.virtual_address,
            section.mapped_size,
            raw_offset,
            raw_size,
            characteristics,
            0,
        ))

    require(lstr_encrypted, ".lstr section not found in the DLL (packer and the DLL disagree)")

    relocations = collect_relocations(layout)
    imports = collect_imports(layout)

    section_table = b"".join(section_records)
    relocation_table = b"".join(struct.pack("<II", rva, 0) for rva in relocations)
    import_table = b"".join(struct.pack("<QQIHH", module_hash, symbol_hash, iat_rva, ordinal, flags)
                            for module_hash, symbol_hash, iat_rva, ordinal, flags in imports)
    hook_table = b"".join(struct.pack("<IIII", hook_id, handler_rva, 0, 0)
                          for hook_id, handler_rva in hooks)

    section_table_offset = ATLX_HEADER_SIZE
    relocation_table_offset = section_table_offset + len(section_table)
    import_table_offset = relocation_table_offset + len(relocation_table)
    hook_table_offset = import_table_offset + len(import_table)
    base_raw_data_offset = align(hook_table_offset + len(hook_table), 16)
    extra_padding = secrets.randbelow(16) * 16 if randomize else 0
    raw_data_offset = base_raw_data_offset + extra_padding
    padding_size = raw_data_offset - (hook_table_offset + len(hook_table))
    padding = secrets.token_bytes(padding_size) if randomize and padding_size else b"\0" * padding_size
    raw_data = b"".join(raw_chunks)
    total_size = raw_data_offset + len(raw_data)

    header = struct.pack(
        "<IHHIIQIIIIIIIIIIII32sHHI",
        ATLX_MAGIC,
        ATLX_VERSION,
        ATLX_HEADER_SIZE,
        total_size,
        layout.image_size,
        layout.image_base,
        entry_rva,
        ATLX_FLAG_ENTRY_ATLAS_START,
        section_table_offset,
        len(section_records),
        relocation_table_offset,
        len(relocations),
        import_table_offset,
        len(imports),
        raw_data_offset,
        len(raw_data),
        hook_table_offset,
        len(hooks),
        hashlib.sha256(layout.data).digest(),
        layout.machine,
        0,
        0,
    )
    return header + section_table + relocation_table + import_table + hook_table + padding + raw_data


def parse_hooks(layout: PELayout) -> list[tuple[int, int]]:
    sections = [section for section in layout.sections if section.name == ATLAS_HOOK_SECTION]
    require(len(sections) <= 1, "duplicate Atlas hook registration section")
    if not sections:
        return []

    section = sections[0]
    require(section.raw_size != 0 and section.raw_size % ATLAS_HOOK_RECORD_SIZE == 0,
            "invalid Atlas hook registration section")

    hooks: list[tuple[int, int]] = []
    ids: set[int] = set()
    for offset in range(section.raw_pointer, section.raw_pointer + section.raw_size,
                        ATLAS_HOOK_RECORD_SIZE):
        hook_id, reserved, handler_va = struct.unpack_from("<IIQ", layout.data, offset)

        if hook_id == 0 and reserved == 0 and handler_va == 0:
            continue

        require(hook_id != 0 and reserved == 0 and handler_va >= layout.image_base,
                "invalid Atlas hook registration")
        handler_rva = handler_va - layout.image_base
        require(handler_rva < layout.image_size, "hook handler outside image")
        require(hook_id not in ids, f"duplicate hook ID 0x{hook_id:08x}")
        require(rva_is_executable(layout, handler_rva),
                f"hook handler 0x{handler_rva:x} is not executable")

        ids.add(hook_id)
        hooks.append((hook_id, handler_rva))

    require(hooks, "empty Atlas hook registration section")
    hooks.sort(key=lambda record: record[0])
    return hooks


def protect_pe(source: Path, map_file: Path, executable: Path, output: Path) -> PELayout:
    require(source.resolve() != output.resolve(), "VMProtect output must not overwrite the input DLL")
    original = parse_pe(source.read_bytes())
    require(original.entry_rva == 0 and not collect_imports(original),
            "VMProtect input must be the freestanding /NOENTRY release DLL")
    symbols = {
        match.group(1): int(match.group(2), 16)
        for match in re.finditer(r"^\s+\w+:\w+\s+(\S+)\s+([0-9A-Fa-f]{16})\s",
                                 map_file.read_text(), re.MULTILINE)
    }
    # Select by this build's linker addresses: VMP does not resolve clang-cl's decorated MAP names.
    procedures = (
        "atlas_start",
        "?startup@session@@QEAA_NXZ",
        "?accept_grant@session@@AEAA_NAEBV?$vector@D@atl@@_K@Z",
        "?worker@session@@CAXPEAX@Z",
        "decrypt_lstr",
    )
    document = ET.Element("Document", Version="2")
    protection = ET.SubElement(document, "Protection", InputFileName=str(source.resolve()),
                               OutputFileName=str(output.resolve()), Options="0", VMCodeSectionName=".vmp", VMComplexity="50")
    entries = ET.SubElement(protection, "Procedures")
    for name in procedures:
        require(name in symbols, f"required VMP procedure is missing from the linker map: {name}")
        require(rva_is_executable(original, symbols[name] - original.image_base),
                f"VMP procedure is outside executable sections: {name}")
        ET.SubElement(entries, "Procedure", Address=str(symbols[name]), Options="0", CompilationType="0")
    require(symbols["atlas_start"] - original.image_base == resolve_export_rva(original, b"atlas_start"),
            "linker map does not match this DLL's atlas_start export")
    output.parent.mkdir(parents=True, exist_ok=True)
    project = source.with_suffix(source.suffix + ".vmp")
    ET.indent(document)
    ET.ElementTree(document).write(project, encoding="utf-8", xml_declaration=True)
    result = subprocess.run(
        [str(executable.resolve()), str(project.resolve()), "-we"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", check=False,
    )
    for line in result.stdout.splitlines():
        if not line.startswith("Registered to:"):
            print(line)
    require(result.returncode == 0, f"VMProtect failed with exit code {result.returncode}")
    protected = parse_pe(output.read_bytes())
    require(protected.entry_rva == 0, "VMProtect introduced an entrypoint that ATLX does not execute")
    require(not collect_imports(protected), "VMProtect introduced imports into the freestanding DLL")
    require(any(section.name.startswith(b".vmp") for section in protected.sections),
            "VMProtect output has no virtualized code section")
    return protected


def main() -> int:
    parser = argparse.ArgumentParser(description="Pack an Atlas PE payload into ATLX v2.")
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--entry", default="atlas_start")
    parser.add_argument("--deterministic", action="store_true")
    parser.add_argument("--vmprotect", type=Path)
    parser.add_argument("--map", dest="map_file", type=Path)
    parser.add_argument("--protected-output", type=Path)
    args = parser.parse_args()

    if args.vmprotect:
        require(args.map_file is not None and args.protected_output is not None,
                "--vmprotect requires --map and --protected-output")
        require(args.output.resolve() not in (args.input.resolve(), args.protected_output.resolve()),
                "ATLX output must be separate from both DLLs")
        layout = protect_pe(args.input, args.map_file, args.vmprotect, args.protected_output)
    else:
        require(args.map_file is None and args.protected_output is None,
                "--map and --protected-output require --vmprotect")
        layout = parse_pe(args.input.read_bytes())
    entry_rva = resolve_export_rva(layout, args.entry.encode("ascii"))
    hooks = parse_hooks(layout)
    blob = build_atlx(layout, entry_rva, hooks, not args.deterministic)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob)
    print(
        f"packed {args.protected_output if args.vmprotect else args.input} -> {args.output} "
        f"({len(blob)} bytes, entry_rva=0x{entry_rva:x}, "
        f"sections={len(layout.sections)}, relocs={len(collect_relocations(layout))}, "
        f"imports={len(collect_imports(layout))}, hooks={len(hooks)})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
