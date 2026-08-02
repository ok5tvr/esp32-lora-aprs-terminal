# Offline Map

The terminal renders offline Web Mercator / XYZ map tiles from the microSD card. It never downloads map data by itself.

## Features

- zoom levels 3 through 18
- center from GPS or saved default position
- touch-drag panning
- own-position marker
- APRS station, object and item symbols
- emergency outline
- recent Trail logger polyline
- progressive tile loading
- missing-tile indication

## Controls

- drag: pan manually
- Up: zoom in
- Down: zoom out
- OK: leave manual mode and recenter on GPS/default position
- Back: return to menu

Status modes:

- `GPS`: follows a current GPS position
- `DEF`: follows the saved default position
- `MAN`: manual center selected by panning

## SD-card tile format

Tiles are stored as raw little-endian RGB565 files:

```text
/MAP/<zoom>/<x>/<y>.rgb
```

Example:

```text
/MAP/13/4398/2785.rgb
```

Each tile must be exactly:

```text
256 x 256 x 2 = 131072 bytes
```

## Converting image tiles

Install Pillow:

```powershell
python -m pip install Pillow
```

Convert a standard XYZ directory:

```powershell
python tools/convert_map_tiles.py `
  --input C:\maps\xyz `
  --output E:\MAP `
  --min-zoom 10 `
  --max-zoom 16
```

Input example:

```text
C:\maps\xyz\13\4398\2785.png
```

Copy the generated `MAP` directory to the root of the microSD card.

Use only map data whose licence permits offline storage and conversion.

## Performance

The map framebuffer uses PSRAM. Tiles are loaded progressively in small strips after radio and GPS processing. The RA-02 uses separate HSPI, while the display and microSD share VSPI.

A missing or damaged tile is shown as a grey checkerboard and does not stop the other terminal functions.
