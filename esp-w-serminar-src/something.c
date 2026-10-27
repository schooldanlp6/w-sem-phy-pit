#include <stdint.h>
#include <HardwareSerial.h›

HardwareSerial radarSerial(2);
#define RADAR_RX 16
#define RADAR_TX 17
#define RADAR_BAUD 256000
struct Target {
int16_t x; int16_t y;
int16_t speed;
vint16_t resolution;
bool active;
Target targets[3];
bool readFrame() {
static vint8_t buf [30];
static int pos = 0;
while (radarSerial. available()) {
vint8_t b = radarSerial. read ();
buf [pos++] = b;
if (pos == 0 && b |= OXAA) continue;
if (pos == 1 8&
b |= OxFF) { pos = 0; continue; }
if (pos == 2 &&
b = 0x03) { pos = 0; continue; }
if (pos == 3 && b |= 0x00) { pos = 0; continue; 3
if (pos == 30) 1
pos = 0;
if (buf[28] |= 0x55  buf[29] |= 0xCC) return false;
for (int i = 0; i < 3; i++) i
int offset = 4 + i * 8;
targets[i].x
= (int16_t) (buf[offset]
| buf [offset + 1]
« 8) ;
targets [il.y
= (int16_t) (buf[offset + 2] | buf[offset + 3] «< 8);
targets [i]. speed
= (int16_t) (buf[offset + 4] | buf [offset + 5] «< 8) ;
targets[i]. resolution = (vint16_t) (buf [offset + 6] | buf [offset + 7] « 8) targets[i]. active
= (targets[i].x != 0  targets[i]. y != 0) ;
return true;
return false;
void printTargets) t
int activeCount = 0;
for (int i = 0; i < 3; i++) {
if (targets[i].active) activeCount++;
}
Serial. println("====
="）；
Serial. printf("Objekte erkannt: %d\n", activeCount) ;
for (int i = 0; i < 3; i++) {
if (targets[i].active) {
// Distanz vom Sensor berechnen


float distance = sqrt(
(float) targets [i].x * targets[i].x +
(float) targets[i]. y * targets[i]. y
) / 1000.0;
// Winkel berechnen
float angle = atan2(targets [i].x, targets[i].y) * 180.0 / PI;
// Bewegungsrichtung
[11:22]Pit: String direction = "";
if (targets[i]. speed > 10)
direction = "bewegt sich weg";
else if (targets[i]. speed < -10) direction = "kommt naeher";
direction = "steht still";
Serial printf ("Objekt %d:\n", i + 1);
Serial printf(" Position : X=%d mm, Y=%d mm\n", targets[i].x, targets[1]. y)
Serial printf(" Distanz : %.2f m\n", distance);
Serial printf(" Winkel : %.1f Grad\n", angle);
Serial printf(" Speed :%d cm/s (%s)\n", targets[1]. speed, direction. c_s)
Serial printf(" Aufloesung: %d mm\n", targets[i]. resolution);
void 100p () {}
if (readFrame()) {