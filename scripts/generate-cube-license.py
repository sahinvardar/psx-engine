#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path

LICENSE_SIZE = 28032
SECTOR_PAYLOAD_SIZE = 2336
SECTOR_DATA_OFFSET = 8
SECTOR_DATA_SIZE = 2048
LOGO_FIRST_SECTOR = 5
LOGO_SECTOR_COUNT = 7
LOGO_SIZE = 0x3278


def build_cube_tmd() -> bytes:
    half_extent = 256
    vertices = (
        (-half_extent, -half_extent, -half_extent),
        (half_extent, -half_extent, -half_extent),
        (-half_extent, half_extent, -half_extent),
        (half_extent, half_extent, -half_extent),
        (half_extent, -half_extent, half_extent),
        (-half_extent, -half_extent, half_extent),
        (half_extent, half_extent, half_extent),
        (-half_extent, half_extent, half_extent),
    )
    faces = (
        ((0, 1, 2, 3), (255, 72, 72)),
        ((4, 5, 6, 7), (72, 192, 255)),
        ((5, 4, 0, 1), (255, 208, 64)),
        ((6, 7, 3, 2), (96, 224, 112)),
        ((0, 2, 5, 7), (208, 96, 255)),
        ((3, 1, 6, 4), (96, 128, 255)),
    )

    primitives = bytearray()
    for indices, color in faces:
        primitives.extend(struct.pack("<BBBB", 5, 3, 1, 0x29))
        primitives.extend(struct.pack("<BBBB", *color, 0x29))
        primitives.extend(struct.pack("<HHHH", *indices))

    vertex_data = bytearray()
    for x, y, z in vertices:
        vertex_data.extend(struct.pack("<hhhh", x, y, z, 0))

    object_table_size = 28
    primitive_offset = object_table_size
    vertex_offset = primitive_offset + len(primitives)
    normal_offset = vertex_offset + len(vertex_data)

    tmd = bytearray(struct.pack("<III", 0x41, 0, 1))
    tmd.extend(
        struct.pack(
            "<IIIIIIi",
            vertex_offset,
            len(vertices),
            normal_offset,
            0,
            primitive_offset,
            len(faces),
            0,
        )
    )
    tmd.extend(primitives)
    tmd.extend(vertex_data)
    return bytes(tmd)


def replace_logo(license_data: bytearray, logo: bytes) -> None:
    if len(logo) > LOGO_SIZE:
        raise ValueError(f"cube TMD is too large: {len(logo)} > {LOGO_SIZE}")

    logo_area = logo + bytes([0xFF]) * (LOGO_SIZE - len(logo))
    logo_area += bytes([0xFF]) * (
        LOGO_SECTOR_COUNT * SECTOR_DATA_SIZE - len(logo_area)
    )

    for sector_index in range(LOGO_SECTOR_COUNT):
        license_sector = LOGO_FIRST_SECTOR + sector_index
        destination = (
            license_sector * SECTOR_PAYLOAD_SIZE + SECTOR_DATA_OFFSET
        )
        source = sector_index * SECTOR_DATA_SIZE
        license_data[destination : destination + SECTOR_DATA_SIZE] = (
            logo_area[source : source + SECTOR_DATA_SIZE]
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Replace a dumped PSX license logo with a cube TMD."
    )
    parser.add_argument("input", type=Path, help="28,032-byte base license")
    parser.add_argument("output", type=Path, help="generated license path")
    args = parser.parse_args()

    license_data = bytearray(args.input.read_bytes())
    if len(license_data) != LICENSE_SIZE:
        raise ValueError(
            f"{args.input} is {len(license_data)} bytes; expected {LICENSE_SIZE}"
        )

    replace_logo(license_data, build_cube_tmd())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(license_data)


if __name__ == "__main__":
    main()
