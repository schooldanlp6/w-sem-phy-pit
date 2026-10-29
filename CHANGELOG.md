# Changelog

## 2026-10-29

- Added a compact layout for 128x32 OLEDs with target count, nearest-target
  distance, angle, movement direction, and UART wiring hints when radar data is
  unavailable.
- Changed OLED setup failures to log the error, clean up initialized display
  resources, and let the radar UART and serial output continue running.
- Updated the display renderer interface and host preview call sites for the
  configured OLED height.
- Added a five-frame rolling median for the nearest-target distance on the OLED
  and in serial output, with resets when the nearest target slot changes, the
  reported distance makes a large jump, or radar data goes stale.
- Documented the filter's response delay and the distinction between radar
  range-bin resolution and guaranteed distance accuracy.
