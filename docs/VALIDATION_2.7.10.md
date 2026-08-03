# Validation 2.7.10

## Scope

Compile compatibility fix for Web OTA on Arduino-ESP32 frameworks where `UpdateClass::write` has the signature `size_t write(uint8_t *data, size_t len)`.

## Change

`OtaService::writeFirmwareBytes` now accepts `std::uint8_t*`. Both callers already provide mutable buffers (`headerBuffer_` and `HTTPUpload::buf`), so no copy or `const_cast` is required.

## Checks

- All `writeFirmwareBytes` declarations and definitions use the same mutable buffer type.
- All call sites provide mutable byte buffers.
- OTA image header validation remains read-only and unchanged.
- Existing host OTA validation and partition tests pass.
