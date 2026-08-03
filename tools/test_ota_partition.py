#!/usr/bin/env python3
from pathlib import Path
import csv

FLASH_SIZE = 0x1000000
EXPECTED = {
    "nvs": (0x9000, 0x5000),
    "otadata": (0xE000, 0x2000),
    "ota_0": (0x10000, 0x700000),
    "ota_1": (0x710000, 0x700000),
    "spiffs": (0xE10000, 0x1E0000),
    "coredump": (0xFF0000, 0x10000),
}

path = Path(__file__).resolve().parents[1] / "partitions_16mb.csv"
rows = []
with path.open(newline="", encoding="utf-8") as handle:
    for raw in handle:
        stripped = raw.strip()
        if not stripped or stripped.startswith("#"):
            continue
        values = [value.strip() for value in next(csv.reader([raw]))]
        name, ptype, subtype, offset_text, size_text = values[:5]
        rows.append((name, ptype, subtype, int(offset_text, 0), int(size_text, 0)))

by_name = {row[0]: row for row in rows}
assert set(EXPECTED).issubset(by_name), f"missing partitions: {set(EXPECTED) - set(by_name)}"
for name, (offset, size) in EXPECTED.items():
    row = by_name[name]
    assert row[3] == offset, f"{name} offset {row[3]:#x} != {offset:#x}"
    assert row[4] == size, f"{name} size {row[4]:#x} != {size:#x}"

ordered = sorted(rows, key=lambda row: row[3])
for previous, current in zip(ordered, ordered[1:]):
    previous_end = previous[3] + previous[4]
    assert previous_end <= current[3], (
        f"partition overlap: {previous[0]} ends {previous_end:#x}, "
        f"{current[0]} starts {current[3]:#x}"
    )
assert max(row[3] + row[4] for row in rows) == FLASH_SIZE
assert by_name["ota_0"][4] == by_name["ota_1"][4]
print("OTA partition layout tests passed")
