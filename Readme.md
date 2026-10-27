# ESP32 DEVKIT1 + HLK-LD2450 radar + OLED proximity display

The ESP32 reads the targets from an **HLK-LD2450** 24 GHz radar (up to 3 people/objects,
position, speed). It prints them on the USB serial console and shows the **distance of the
nearest target** on a small **SSD1306 OLED**.

Built with **ESP-IDF v6.0.1** (plain C, no Arduino).

---

## 1. What you need

| Part | Notes |
|---|---|
| ESP32 DEVKIT1 (DOIT ESP32 DevKit V1, ESP32-WROOM-32) | any ESP32-WROOM-32 board works |
| HLK-LD2450 radar module | comes with a 4-pin **1.25 mm** JST cable (very small, *not* 2.54 mm Dupont) |
| SSD1306 OLED, I2C, 128x64 (0.96") | 4 pins: GND, VCC, SCL, SDA. 128x32 (0.91") also works, see `make menuconfig` |
| Micro-USB **data** cable | charge-only cables are a common reason why no serial port shows up |
| Breadboard + jumper wires | |

---

## 2. Wiring

```
                 ESP32 DEVKIT1
               +---------------+
   LD2450 5V --| VIN       3V3 |-- OLED VCC
   LD2450 GND--| GND       GND |-- OLED GND
               |               |
               |   RX2 (GPIO16)|-- LD2450 TX
               |   TX2 (GPIO17)|-- LD2450 RX
               |               |
               |   D22 (GPIO22)|-- OLED SCL
               |   D21 (GPIO21)|-- OLED SDA
               +---------------+
                  [ USB to PC ]
```

### HLK-LD2450 radar -> ESP32

| LD2450 pin | ESP32 DEVKIT1 pin | Why |
|---|---|---|
| **5V**  | **VIN** | The radar needs 5 V. VIN carries the USB 5 V when the board is powered over USB. |
| **GND** | **GND** | common ground |
| **TX**  | **RX2 / GPIO16** | TX always goes to RX (crossed) |
| **RX**  | **TX2 / GPIO17** | RX always goes to TX (crossed) |

### SSD1306 OLED (I2C) -> ESP32

| OLED pin | ESP32 DEVKIT1 pin |
|---|---|
| **VCC** | **3V3** |
| **GND** | **GND** |
| **SCL** | **D22 / GPIO22** |
| **SDA** | **D21 / GPIO21** |

All pins can be changed in `make menuconfig` under **"LD2450 Radar"** and **"OLED Display"**.

---

## 3. Things to watch out for

1. **TX/RX are crossed.** Radar **TX** goes to ESP32 **RX2 (GPIO16)**. If you swap them you get
   "Keine Radardaten" and the OLED shows the wiring hint screen.
2. **The radar needs 5 V, but its UART uses 3.3 V.** Power it from **VIN** (5 V), never from 3V3.
   The data lines can go straight to the ESP32, so you don't need a level shifter.
3. **The OLED needs 3.3 V.** Most modules accept 3.3–5 V, but 3V3 is the safe choice.
4. **Pin order differs between OLED modules.** Some are `GND VCC SCL SDA`, others `VCC GND SCL SDA`.
   Read the labels on the board, not a picture.
5. **OLED I2C address** is usually `0x3C`, sometimes `0x3D` (solder jumper on the back).
   At boot the log says `No OLED at I2C address 0x3C` if nothing answers. Change it in menuconfig.
6. **Baud rate is 256000.** That's the LD2450 factory default. If someone changed it with the
   HLK app, set the new value in menuconfig (`Radar baud rate`).
7. **The LD2450's Bluetooth is on by default.** If the HLKRadarTool phone app is connected,
   the module still sends UART data. If values look strange, check the app for changed settings.
8. **Mounting:** the radar looks forward from the side with the antenna patterns, with about ±60° field of
   view and roughly 6 m range. Y is the distance straight ahead and X is the sideways offset.
   It detects **moving or breathing people** better than static objects.
9. **The ESP32 module must be WROOM, not WROVER.** On WROVER boards GPIO16/17 are used by the PSRAM.
   The DEVKIT1 uses a WROOM-32, so this is fine.
10. **Do not use GPIO 6–11** for anything (internal flash).
11. **No serial port on Linux?** Check `make ports`. Then add yourself to the `dialout` group
    (`sudo usermod -aG dialout $USER`, then log out and in again). Also try another USB cable.
12. **Flashing hangs at "Connecting..."?** Hold the **BOOT** button until the upload starts.

---

## 4. Build, flash, run

Requires ESP-IDF v6.x in `~/esp-idf` (the `get_idf` alias in `~/.bashrc` sources
`~/esp-idf/export.sh`). The Makefile sources it automatically if `idf.py` is not on your
PATH yet, so `get_idf` is optional.

```bash
make                    # build the firmware
make ports              # find the serial port of the board
make flash PORT=/dev/ttyUSB0
make monitor PORT=/dev/ttyUSB0   # Ctrl+] to quit
make flash-monitor      # both in one go
make menuconfig         # change pins, OLED address/height/rotation, baud rate
```

Without any hardware:

```bash
make test               # unit tests of the radar parser + display code on the PC (gcc)
make preview            # draws the OLED screens as ASCII art in the terminal
```

Other targets: `make size`, `make clean`, `make fullclean` (deletes `build/`),
`make erase` (erases the whole flash of the board), `make test-clean`, `make help`.

Use a different IDF installation: `make IDF_EXPORT=/path/to/esp-idf/export.sh`.

---

## 5. What you see

### OLED (128x64)

```
Ziele: 3              LD2450     <- number of targets
         0.91m                   <- distance of the NEAREST target (big)
[##################       ]      <- proximity bar: full = close, empty = 6 m+
 1  2.52m    7d ---              <- one line per target:
>2  0.91m   -9d nah                 '>' = nearest, distance, angle,
 3  3.23m   22d weg                 nah = approaching, weg = moving away, --- = still
```

If no frame arrives for 1 s the OLED shows `----` and the wiring hint
`Radar TX -> GPIO16 / Radar RX -> GPIO17`.

### Serial monitor (115200 baud, at most every 0.5 s)

```
====================
Objekte erkannt: 1
Objekt 1:
  Position  : X=-782 mm, Y=1713 mm
  Distanz   : 1.88 m
  Winkel    : -24.5 Grad
  Speed     : -16 cm/s (kommt naeher)
  Aufloesung: 320 mm
```

---

## 6. Project layout

```
Makefile               build / flash / monitor / test wrapper around idf.py
CMakeLists.txt         ESP-IDF project file
sdkconfig.defaults     target esp32, 4 MB flash
main/
  main.c               UART setup, main loop, serial output
  ld2450.c/.h          LD2450 frame parser (no ESP dependencies -> host-testable)
  display.c/.h         128x64 frame buffer, text, screen layout (host-testable)
  font5x7.h            5x7 ASCII font
  oled.c/.h            SSD1306 over I2C via esp_lcd
  Kconfig.projbuild    menuconfig options (pins, baud, OLED)
test/
  Makefile             host build with gcc
  test_ld2450.c        parser tests (datasheet example frame) + ASCII preview
esp-w-serminar-src/    the ORIGINAL code, left unchanged for reference
```

---

## 7. What was wrong with the original code

`esp-w-serminar-src/something.c` was an Arduino sketch copied from a screenshot (OCR), and it
could never compile or work:

- **OCR garbage:** `<HardwareSerial.h›`, `vint8_t`/`vint16_t` (for `uint8_t`), `OXAA`/`OxFF`,
  `«`/`«<` (for `<<`), `8&` (for `&&`), `3`/`1`/`i`/`t` instead of `}`/`{`, `targets[1]`/`[il`
  instead of `targets[i]`, `Serial printf`, `void 100p()`, `printTargets)`, `c_s`,
  `"===="）；`, a pasted chat line `[11:22]Pit:`, and the file ends in the middle of `loop()`.
- **`|=` instead of `!=`** in every header/tail check. That assigns instead of comparing.
- **Missing `||`** in `buf[28] != 0x55 || buf[29] != 0xCC` and in `x != 0 || y != 0`.
- **Header check off by one:** `buf[pos++] = b` ran *before* `if (pos == 0 ...)`, so the
  check `pos == 0` was never true. The parser could never lock onto a frame.
- **Wrong number decoding:** the LD2450 does **not** send normal `int16_t`. Bit 15 = 1 means
  positive, 0 means negative, and the rest is the magnitude. A target 1.7 m in front came
  out as Y = -31055 mm. This is fixed in `ld2450_decode_s16()` and checked against the datasheet
  example frame in `make test`.
- **Missing `else`:** direction was always overwritten with `"steht still"`.
- **No `setup()`:** `Serial.begin()` and `radarSerial.begin(256000, SERIAL_8N1, 16, 17)`
  were never called. `HardwareSerial.h` in the folder is an empty file.
- Printing on every frame (10/s) floods the console. It is now limited to every 0.5 s.

The new code is a native ESP-IDF project. The original folder was left untouched.

---

## 8. Hardware test results and accuracy (Messungenauigkeit)

Tested on 2026-10-27 with an ESP32 DEVKIT1 (ESP32 rev 3.1, 4 MB flash, CP2102 USB chip on
`/dev/ttyUSB0`). Flashing worked without pressing BOOT, and the board booted cleanly.

### What we saw

| Test | Distance | Angle | Result |
|---|---|---|---|
| 1: target close to the sensor | 0.2–0.6 m (some up to 1.1 m) | up to ±54° | values worked but felt jumpy |
| 2: target further away, more in front | 0.36–1.5 m | −52° to +31° | noticeably better |

In both tests the readings changed smoothly (a few cm between readings 0.5 s apart).
That means the decoding is correct. The poor feel came from **how the sensor was used**.

### How to get good values

- **Keep targets between about 0.5 and 6 m.** Very close up (under about 0.5 m) the LD2450 is unreliable.
- **Stay in front of the sensor.** The field of view is about ±60°, and accuracy drops clearly
  toward the edges. Angles near 0° are best.
- **Mount it at chest height,** facing the room, not a nearby wall.

### What the values mean

- **Distance** is calculated from X and Y. Y is straight ahead and X is the sideways offset.
- **Speed** is only movement **toward or away from** the sensor (Doppler), in steps of
  8 cm/s. Walking sideways shows as 0 ("steht still").
- **Aufloesung (resolution)** is always 360 mm. This is the radar's fixed distance step
  (roughly how far apart two targets must be to be told apart). It is **not** the measurement
  error of a single reading.

### How to measure the Messungenauigkeit

The module does not report its own error, so you measure it:

1. Mark distances on the floor with a tape measure, straight in front of the sensor
   (e.g. 1 m, 2 m, 3 m).
2. Stand **still** at one mark and record about 50 readings (≈ 25 s with `make monitor`).
3. For each distance calculate:
   - **mean − true distance** = systematic error. It is the same every time, so you can correct for it.
   - **standard deviation** = random jitter. This is the "±" you write after a value.
4. Repeat at different distances and angles. The error usually grows with distance and angle.

Not built yet, but possible: a measuring mode that shows mean ± standard deviation on the OLED,
and smoothing of the displayed distance.

### Humans or objects?

The LD2450 is built to track **people**. More precisely, it tracks **moving** targets:

- **Things that stand still are filtered out on purpose:** walls, furniture, boxes. A person
  standing still is usually still detected (breathing, small movements) but can drop out
  after a while. In test 2, about half of the readings had no target.
- **It cannot tell what is moving.** A fan, a swinging door, a curtain or a pet shows up
  just like a person.

To also detect objects that don't move, add a second sensor:

| Sensor | Range | Accuracy | Notes |
|---|---|---|---|
| VL53L1X (laser time-of-flight) | ~4 m | mm to cm | measures in one direction only; can share the OLED's I2C wires (SDA 21 / SCL 22) |
| HC-SR04 (ultrasonic) | ~4 m | ~1 cm | cheap, wide beam; soft materials such as fabric absorb the sound |

Combining them works well: the LD2450 tracks people in the room, and the second sensor gives
the exact distance to whatever is straight in front.

---

## 9. Known quirks

### Of the LD2450 sensor

- **A target is the center of a moving cluster, not a single point (and not a sum).** A person
  reflects the radar from many points (torso, arms, legs). The LD2450 groups nearby
  *moving* reflection points into one target and reports roughly their center. If one part of
  the body moves more, for example an arm, the center shifts even though the person stays in place.
- **How big is that jitter?** In our test, the distance changed by only **about 1–3 cm per
  reading** (0.5 s apart) during calm phases (e.g. 249 → 240 → 230 → 222 → 213 → 209 mm).
  Bigger jumps of 10–30 cm came with real movement, very close range (< 0.5 m) or wide angles
  (> ±45°). For exact numbers, do the standing-still test in section 8.
- **Targets close together merge.** Two people or objects closer together than about 360 mm can
  become one target, with its center somewhere between them.
- **Ghost targets.** Walls, metal and windows reflect radar waves, which can create short-lived
  phantom targets, e.g. "behind" a wall.
- **Static things disappear.** Furniture is filtered out. A person standing completely still is
  usually still detected (breathing) but can drop out after a while.
- **It's tuned for humans, but sees any moving thing that reflects radar.** It detects moving
  hands, pets, fans and doors (metal especially strongly), and can't tell them apart from people.
  Moving wood, plastic or fabric reflects only weakly.

### Of this firmware

- **The large OLED number can jump between targets.** It always shows the *nearest* of up to 3
  targets. If two targets are at similar distances, "nearest" can switch from one reading
  to the next, and the number jumps although nothing moved. Possible fix (not built yet):
  averaging, or staying on the same target unless another is clearly closer.
- **Serial output is limited to every 0.5 s.** The radar sends about 10 frames/s, so the serial log
  shows only about every 5th frame. The OLED updates every 0.1 s.
