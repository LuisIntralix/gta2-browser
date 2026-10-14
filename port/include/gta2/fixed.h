/* GTA2 browser port — fixed-width numerics and little-endian readers (T04)
 *
 * Binding decisions: D05 (numeric compatibility and RNG), D03 (boundary).
 * Contract: docs/contracts/simulation-tick-v1.md, docs/contracts/platform-abi-v1.md
 *
 * This header contains only data-layout contracts and pure serialization
 * primitives. Fixed-point *arithmetic* is specified by D05 and implemented
 * under port/core/numeric/ by T05; it is deliberately not defined here.
 * Nothing in this header emulates a platform or hides game behavior.
 */
#ifndef GTA2_FIXED_H
#define GTA2_FIXED_H

#include "gta2/abi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- Fixed-point contract (D05) ---------------- */
#define GTA2_FIX16_FRACTIONAL_BITS   14u
#define GTA2_FIX16_ONE               16384        /* 1 << 14, signed int32 storage */
#define GTA2_FIX16_MUL_DIVISOR       16384
#define GTA2_FIX16_MIN              (-2147483647 - 1)
#define GTA2_FIX16_MAX               2147483647

/* Wire representation: int32 little-endian raw value, 14 fractional bits.
 * Rounding and narrowing happen only in explicit helpers (D05). */
typedef int32_t  gta2_fix16;

/* 16-bit angle turn, used by reconstructed angle code (ang16). */
#define GTA2_ANG16_ONE               65536u
typedef uint16_t gta2_ang16;

/* Q15 quantized aim component: -32768..32767 maps to -1.0..~1.0.
 * (0, 0) means "no aim", which is not the same as aim at origin. */
#define GTA2_Q15_ONE                 32768
typedef int16_t  gta2_q15;

/* Every multi-byte field crossing the boundary or an asset/save/network
 * stream is little-endian, explicitly, never by host struct layout. */
#define GTA2_BYTE_ORDER_LITTLE 1u
#define GTA2_FILE_OFFSET_BYTES 4u   /* file offsets are uint32, never pointers */

/* ---------------- Unchecked accessors ---------------- */
static inline uint16_t gta2_le_u16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline uint32_t gta2_le_u32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline int32_t gta2_le_i32(const uint8_t* p) {
    return (int32_t)gta2_le_u32(p);
}
static inline void gta2_le_put_u16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}
static inline void gta2_le_put_u32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}
static inline void gta2_le_put_i32(uint8_t* p, int32_t v) {
    gta2_le_put_u32(p, (uint32_t)v);
}

/* ---------------- Bounds-checked reader ---------------- */
/* Offsets are uint32 and validated against the buffer length before any
 * read (D05). A short read reports GTA2_STATUS_TRUNCATED and leaves the
 * cursor unchanged. */
typedef struct gta2_reader {
    const uint8_t* data;
    uint32_t       length;
    uint32_t       offset;
} gta2_reader;

static inline void gta2_reader_init(gta2_reader* r, const uint8_t* data, uint32_t length) {
    r->data = data;
    r->length = length;
    r->offset = 0u;
}

static inline int gta2_reader_has(const gta2_reader* r, uint32_t bytes) {
    return (r->offset <= r->length) && (bytes <= (r->length - r->offset));
}

static inline gta2_status gta2_reader_bytes(gta2_reader* r, uint32_t bytes,
                                            const uint8_t** out) {
    if (out == NULL) {
        return GTA2_STATUS_INVALID_ARGUMENT;
    }
    if (!gta2_reader_has(r, bytes)) {
        return GTA2_STATUS_TRUNCATED;
    }
    *out = r->data + r->offset;
    r->offset += bytes;
    return GTA2_STATUS_OK;
}

static inline gta2_status gta2_reader_u16(gta2_reader* r, uint16_t* out) {
    const uint8_t* p = NULL;
    gta2_status s;
    if (out == NULL) {
        return GTA2_STATUS_INVALID_ARGUMENT;
    }
    s = gta2_reader_bytes(r, 2u, &p);
    if (s != GTA2_STATUS_OK) {
        return s;
    }
    *out = gta2_le_u16(p);
    return GTA2_STATUS_OK;
}

static inline gta2_status gta2_reader_u32(gta2_reader* r, uint32_t* out) {
    const uint8_t* p = NULL;
    gta2_status s;
    if (out == NULL) {
        return GTA2_STATUS_INVALID_ARGUMENT;
    }
    s = gta2_reader_bytes(r, 4u, &p);
    if (s != GTA2_STATUS_OK) {
        return s;
    }
    *out = gta2_le_u32(p);
    return GTA2_STATUS_OK;
}

static inline gta2_status gta2_reader_i32(gta2_reader* r, int32_t* out) {
    uint32_t raw = 0u;
    gta2_status s = gta2_reader_u32(r, &raw);
    if (s != GTA2_STATUS_OK) {
        return s;
    }
    if (out == NULL) {
        return GTA2_STATUS_INVALID_ARGUMENT;
    }
    *out = (int32_t)raw;
    return GTA2_STATUS_OK;
}

/* ---------------- Capacity-checked writer ---------------- */
typedef struct gta2_writer {
    uint8_t* data;
    uint32_t capacity;
    uint32_t offset;
} gta2_writer;

static inline void gta2_writer_init(gta2_writer* w, uint8_t* data, uint32_t capacity) {
    w->data = data;
    w->capacity = capacity;
    w->offset = 0u;
}

static inline int gta2_writer_has(const gta2_writer* w, uint32_t bytes) {
    return (w->offset <= w->capacity) && (bytes <= (w->capacity - w->offset));
}

static inline gta2_status gta2_writer_bytes(gta2_writer* w, const uint8_t* src, uint32_t bytes) {
    uint32_t i;
    if (src == NULL) {
        return GTA2_STATUS_INVALID_ARGUMENT;
    }
    if (!gta2_writer_has(w, bytes)) {
        return GTA2_STATUS_LIMIT; /* caller retries with the required size */
    }
    for (i = 0u; i < bytes; ++i) {
        w->data[w->offset + i] = src[i];
    }
    w->offset += bytes;
    return GTA2_STATUS_OK;
}

static inline gta2_status gta2_writer_u16(gta2_writer* w, uint16_t v) {
    uint8_t tmp[2];
    gta2_le_put_u16(tmp, v);
    return gta2_writer_bytes(w, tmp, 2u);
}

static inline gta2_status gta2_writer_u32(gta2_writer* w, uint32_t v) {
    uint8_t tmp[4];
    gta2_le_put_u32(tmp, v);
    return gta2_writer_bytes(w, tmp, 4u);
}

static inline gta2_status gta2_writer_i32(gta2_writer* w, int32_t v) {
    return gta2_writer_u32(w, (uint32_t)v);
}

/* Zero padding to an 8-byte boundary, as required by the pack and save
 * framing (D06, D09). Returns the number of padding bytes written. */
#define GTA2_ALIGNMENT_8 8u
static inline uint32_t gta2_align8_capacity(uint32_t bytes) {
    return (uint32_t)((bytes + (GTA2_ALIGNMENT_8 - 1u)) & ~(GTA2_ALIGNMENT_8 - 1u));
}
static inline gta2_status gta2_writer_pad8(gta2_writer* w, uint32_t* out_padding) {
    uint32_t aligned = gta2_align8_capacity(w->offset);
    uint32_t pad = aligned - w->offset;
    uint32_t i;
    if (pad == 0u) {
        if (out_padding != NULL) {
            *out_padding = 0u;
        }
        return GTA2_STATUS_OK;
    }
    if (!gta2_writer_has(w, pad)) {
        return GTA2_STATUS_LIMIT;
    }
    for (i = 0u; i < pad; ++i) {
        w->data[w->offset + i] = 0u;
    }
    w->offset = aligned;
    if (out_padding != NULL) {
        *out_padding = pad;
    }
    return GTA2_STATUS_OK;
}

#if defined(__cplusplus)
#define GTA2_FIXED_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define GTA2_FIXED_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

GTA2_FIXED_ASSERT(sizeof(gta2_fix16) == 4, "gta2_fix16 is int32");
GTA2_FIXED_ASSERT(sizeof(gta2_ang16) == 2, "gta2_ang16 is uint16");
GTA2_FIXED_ASSERT(sizeof(gta2_q15) == 2, "gta2_q15 is int16");
GTA2_FIXED_ASSERT(GTA2_FIX16_ONE == (1 << GTA2_FIX16_FRACTIONAL_BITS),
                  "fix16 one must equal 1 << fractional bits");
GTA2_FIXED_ASSERT(GTA2_FIX16_MUL_DIVISOR == GTA2_FIX16_ONE,
                  "fix16 multiply divisor is the fixed-point scale");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GTA2_FIXED_H */
