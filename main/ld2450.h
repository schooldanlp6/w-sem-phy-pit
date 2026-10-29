/*
 * HLK-LD2450 24 GHz radar - frame parser (hardware independent).
 *
 * Report frame (30 bytes, 256000 baud 8N1, ~10 frames/s):
 *   AA FF 03 00 | target1 (8) | target2 (8) | target3 (8) | 55 CC
 * Each target, little endian:
 *   x (2) | y (2) | speed (2) | resolution (2)
 * x, y and speed use "sign + magnitude": bit15 = 1 -> positive,
 * bit15 = 0 -> negative, bits 0..14 = absolute value.
 * x/y in mm, speed in cm/s, resolution (uint16) in mm.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LD2450_FRAME_LEN   30
#define LD2450_MAX_TARGETS 3
#define LD2450_RANGE_FILTER_SAMPLES 5

typedef struct {
    int16_t x_mm;           /* lateral offset from the sensor axis */
    int16_t y_mm;           /* distance in front of the sensor */
    int16_t speed_cms;      /* > 0: moving away, < 0: approaching */
    uint16_t resolution_mm;
    bool active;
} ld2450_target_t;

typedef struct {
    ld2450_target_t t[LD2450_MAX_TARGETS];
} ld2450_frame_t;

typedef struct {
    uint8_t buf[LD2450_FRAME_LEN];
    int pos;
    uint32_t bad_frames;    /* frames with a broken tail */
} ld2450_parser_t;

typedef struct {
    uint32_t samples_mm[LD2450_RANGE_FILTER_SAMPLES];
    uint8_t count;
    uint8_t next;
} ld2450_range_filter_t;

void ld2450_parser_init(ld2450_parser_t *p);

/* Feed one received byte. Returns true and fills *out when a complete,
 * valid frame was received. */
bool ld2450_feed(ld2450_parser_t *p, uint8_t b, ld2450_frame_t *out);

/* Decode a complete 30 byte frame. Returns false if header/tail are wrong. */
bool ld2450_decode_frame(const uint8_t *buf, ld2450_frame_t *out);

int16_t ld2450_decode_s16(uint8_t lo, uint8_t hi);

float ld2450_distance_m(const ld2450_target_t *t);
float ld2450_angle_deg(const ld2450_target_t *t);
int ld2450_active_count(const ld2450_frame_t *f);
int ld2450_nearest(const ld2450_frame_t *f);   /* index, -1 if none */
void ld2450_range_filter_reset(ld2450_range_filter_t *filter);
uint32_t ld2450_range_filter_update(ld2450_range_filter_t *filter, uint32_t distance_mm);

/* Movement direction as text: long form for serial, short for the OLED. */
const char *ld2450_direction(const ld2450_target_t *t);
const char *ld2450_direction_short(const ld2450_target_t *t);
