#include "ld2450.h"

#include <math.h>
#include <string.h>

#define SPEED_THRESHOLD_CMS 10

static const uint8_t HEADER[4] = {0xAA, 0xFF, 0x03, 0x00};
static const uint8_t TAIL[2] = {0x55, 0xCC};

void ld2450_parser_init(ld2450_parser_t *p)
{
    memset(p, 0, sizeof(*p));
}

int16_t ld2450_decode_s16(uint8_t lo, uint8_t hi)
{
    uint16_t raw = (uint16_t)(lo | (hi << 8));
    int16_t mag = (int16_t)(raw & 0x7FFF);
    return (raw & 0x8000) ? mag : (int16_t)-mag;
}

bool ld2450_decode_frame(const uint8_t *buf, ld2450_frame_t *out)
{
    if (memcmp(buf, HEADER, sizeof(HEADER)) != 0 ||
        memcmp(buf + LD2450_FRAME_LEN - 2, TAIL, sizeof(TAIL)) != 0) {
        return false;
    }
    for (int i = 0; i < LD2450_MAX_TARGETS; i++) {
        const uint8_t *d = buf + 4 + i * 8;
        ld2450_target_t *t = &out->t[i];
        t->x_mm = ld2450_decode_s16(d[0], d[1]);
        t->y_mm = ld2450_decode_s16(d[2], d[3]);
        t->speed_cms = ld2450_decode_s16(d[4], d[5]);
        t->resolution_mm = (uint16_t)(d[6] | (d[7] << 8));
        t->active = (t->x_mm != 0 || t->y_mm != 0);
    }
    return true;
}

bool ld2450_feed(ld2450_parser_t *p, uint8_t b, ld2450_frame_t *out)
{
    if (p->pos < (int)sizeof(HEADER)) {
        if (b == HEADER[p->pos]) {
            p->buf[p->pos++] = b;
        } else {
            /* resync: a mismatching byte may itself start a new header */
            p->pos = 0;
            if (b == HEADER[0]) {
                p->buf[p->pos++] = b;
            }
        }
        return false;
    }

    p->buf[p->pos++] = b;
    if (p->pos < LD2450_FRAME_LEN) {
        return false;
    }
    p->pos = 0;
    if (!ld2450_decode_frame(p->buf, out)) {
        p->bad_frames++;
        return false;
    }
    return true;
}

float ld2450_distance_m(const ld2450_target_t *t)
{
    float x = t->x_mm, y = t->y_mm;
    return sqrtf(x * x + y * y) / 1000.0f;
}

float ld2450_angle_deg(const ld2450_target_t *t)
{
    return atan2f((float)t->x_mm, (float)t->y_mm) * 180.0f / (float)M_PI;
}

int ld2450_active_count(const ld2450_frame_t *f)
{
    int n = 0;
    for (int i = 0; i < LD2450_MAX_TARGETS; i++) {
        if (f->t[i].active) {
            n++;
        }
    }
    return n;
}

int ld2450_nearest(const ld2450_frame_t *f)
{
    int best = -1;
    float best_d = 0;
    for (int i = 0; i < LD2450_MAX_TARGETS; i++) {
        if (!f->t[i].active) {
            continue;
        }
        float d = ld2450_distance_m(&f->t[i]);
        if (best < 0 || d < best_d) {
            best = i;
            best_d = d;
        }
    }
    return best;
}

const char *ld2450_direction(const ld2450_target_t *t)
{
    if (t->speed_cms > SPEED_THRESHOLD_CMS) {
        return "bewegt sich weg";
    } else if (t->speed_cms < -SPEED_THRESHOLD_CMS) {
        return "kommt naeher";
    }
    return "steht still";
}

const char *ld2450_direction_short(const ld2450_target_t *t)
{
    if (t->speed_cms > SPEED_THRESHOLD_CMS) {
        return "weg";
    } else if (t->speed_cms < -SPEED_THRESHOLD_CMS) {
        return "nah";
    }
    return "---";
}
