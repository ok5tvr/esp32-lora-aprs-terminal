# APRS tracker symbols

The **Tracker** screen contains a touch dropdown for the symbol transmitted in
position packets. The selected value is stored under the `trksym` key in the
`loraaprs` NVS namespace and survives restart or power loss.

| Selection | APRS table/code |
|---|---|
| Car | `/>` |
| Pedestrian | `/[` |
| Bicycle | `/b` |
| Motorcycle | `/<` |
| Fixed QTH / house | `/-` |
| Boat | `/s` |
| Aircraft | `/^` |
| Balloon | `/O` |
| Weather station | `/_` |
| Generic point | `//` |
| LoRa iGate | `L&` |

The same table/code pair is passed to the APRS encoder for both normal and
Base-91 compressed positions. Saving a new symbol increments the settings
revision, so the tracker is re-armed and the next packet uses the new symbol.

The definitions and dropdown order are centralized in:

```text
src/app/tracker_symbols.h
```

Add future symbols only before `TrackerSymbol::Count` and append matching
entries to `TRACKER_SYMBOL_DEFINITIONS` and the dropdown option string. The
compile-time assertion checks that the enum and definition array have the same
number of entries.
