# APRS Beacon

Firmware v2.7.9 provides a dedicated page for sending a single APRS position beacon without enabling periodic tracking.

## Position source

Choose one of two sources:

- **GPS**: uses the current valid GPS fix. Course and speed are included when both values are valid. Transmission is rejected when the receiver has no current fix.
- **Default**: uses the latitude and longitude saved on the Settings page.

## APRS path

- **DIRECT**: no path is added to the TNC2 header.
- **WIDE1-1**: requests one local fill-in hop.
- **WIDE2-2**: requests up to two regional hops.

Choose a path appropriate for the local LoRa APRS network and operating rules. Avoid unnecessary wide paths.

## Comment

The Beacon comment accepts 1 to 48 printable characters and is stored in ESP32 NVS. It remains available after reset or power loss.

## Format and symbol

The dedicated Beacon intentionally uses the compressed/uncompressed format and APRS symbol selected on the Tracker page. Its position source, path and comment remain independent from Tracker settings.

## Save and send

- **Save beacon settings** stores the current fields without transmitting.
- **Send beacon now** stores the fields, builds the position frame and queues it with manual-beacon priority.
- The physical **OK** action on the Beacon screen also sends the packet.

The send operation fails safely if the radio is unavailable, a GPS source has no fix, the APRS frame cannot be encoded or the central TX queue is full.

Example:

```text
OK5TVR-15>APRS,WIDE1-1:=4947.18N/01317.10E> QTH beacon
```
