#ifndef _ZCM_LIB_INLINE_H
#define _ZCM_LIB_INLINE_H

#include "zcm/zcm_base_types.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define   ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS (1)
#define  ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS (2)
#define  ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS (4)
#define  ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS (8)
#define  ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS (4)
#define ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS (8)

typedef struct ___zcm_hash_ptr __zcm_hash_ptr;
struct ___zcm_hash_ptr
{
    const __zcm_hash_ptr *parent;
    void *v;
};

/**
 * BOOLEAN
 */
#define __boolean_hash_recursive __int8_t_hash_recursive
#define __boolean_decode_array_cleanup __int8_t_decode_array_cleanup
#define __boolean_encoded_array_size __int8_t_encoded_array_size
#define __boolean_encode_array __int8_t_encode_array
#define __boolean_decode_array __int8_t_decode_array
#define __boolean_encode_little_endian_array __int8_t_encode_little_endian_array
#define __boolean_decode_little_endian_array __int8_t_decode_little_endian_array
#define __boolean_clone_array __int8_t_clone_array

/**
 * BYTE
 */
#define __byte_hash_recursive(p) 0
#define __byte_decode_array_cleanup(p, sz) {}

static inline zuint32_t __byte_encoded_array_size(const zuint8_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __byte_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zuint8_t *p, zuint32_t elements)
{
    if (maxlen < elements) return -1;

    zuint8_t *buf = (zuint8_t*) _buf;
    memcpy(&buf[offset], p, elements);

    return elements;
}

static inline zint32_t __byte_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zuint8_t *p, zuint32_t elements)
{
    if (maxlen < elements) return -1;

    zuint8_t *buf = (zuint8_t*) _buf;
    memcpy(p, &buf[offset], elements);

    return elements;
}

static inline zint32_t __byte_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zuint8_t *p, zuint32_t elements)
{
    return __byte_encode_array(_buf, offset, maxlen, p, elements);
}

static inline zint32_t __byte_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zuint8_t *p, zuint32_t elements)
{
    return __byte_decode_array(_buf, offset, maxlen, p, elements);
}

static inline zuint32_t __byte_clone_array(const zuint8_t *p, zuint8_t *q, zuint32_t elements)
{
    // Intentionally not using ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS
    zuint32_t n = elements * sizeof(zuint8_t);
    memcpy(q, p, n);
    return n;
}

/**
 * INT8_T
 */
#define __int8_t_hash_recursive(p) 0
#define __int8_t_decode_array_cleanup(p, sz) {}

static inline zuint32_t __int8_t_encoded_array_size(const zint8_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __int8_t_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint8_t *p, zuint32_t elements)
{
    if (maxlen < elements) return -1;

    zint8_t *buf = (zint8_t*) _buf;
    memcpy(&buf[offset], p, elements);

    return elements;
}

static inline zint32_t __int8_t_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint8_t *p, zuint32_t elements)
{
    if (maxlen < elements) return -1;

    zint8_t *buf = (zint8_t*) _buf;
    memcpy(p, &buf[offset], elements);

    return elements;
}

static inline zint32_t __int8_t_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint8_t *p, zuint32_t elements)
{
    return __int8_t_encode_array(_buf, offset, maxlen, p, elements);
}

static inline zint32_t __int8_t_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint8_t *p, zuint32_t elements)
{
    return __int8_t_decode_array(_buf, offset, maxlen, p, elements);
}

static inline zuint32_t __int8_t_clone_array(const zint8_t *p, zint8_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zint8_t);
    memcpy(q, p, n);
    return n;
}

/**
 * INT16_T
 */
#define __int16_t_hash_recursive(p) 0
#define __int16_t_decode_array_cleanup(p, sz) {}

static inline zuint32_t __int16_t_encoded_array_size(const zint16_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __int16_t_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint16_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint16_t v = (zuint16_t) p[element];
        buf[pos++] = (v>>8) & 0xff;
        buf[pos++] = (v & 0xff);
    }

    return total_size;
}

static inline zint32_t __int16_t_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint16_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        p[element] = (buf[pos]<<8) + buf[pos+1];
        pos += 2;
    }

    return total_size;
}

static inline zint32_t __int16_t_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint16_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint16_t v = (zuint16_t) p[element];
        buf[pos++] = (v & 0xff);
        buf[pos++] = (v>>8) & 0xff;
    }

    return total_size;
}

static inline zint32_t __int16_t_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint16_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT16_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        p[element] = (buf[pos+1]<<8) + buf[pos];
        pos+=2;
    }

    return total_size;
}

static inline zuint32_t __int16_t_clone_array(const zint16_t *p, zint16_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zint16_t);
    memcpy(q, p, n);
    return n;
}

/**
 * INT32_T
 */
#define __int32_t_hash_recursive(p) 0
#define __int32_t_decode_array_cleanup(p, sz) {}

static inline zuint32_t __int32_t_encoded_array_size(const zint32_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __int32_t_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint32_t v = (zuint32_t) p[element];
        buf[pos++] = (v>>24)&0xff;
        buf[pos++] = (v>>16)&0xff;
        buf[pos++] = (v>>8)&0xff;
        buf[pos++] = (v & 0xff);
    }

    return total_size;
}

static inline zint32_t __int32_t_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        p[element] = (((zuint32_t)buf[pos+0])<<24) +
                     (((zuint32_t)buf[pos+1])<<16) +
                     (((zuint32_t)buf[pos+2])<<8) +
                      ((zuint32_t)buf[pos+3]);
        pos+=4;
    }

    return total_size;
}

static inline zint32_t __int32_t_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint32_t v = (zuint32_t) p[element];
        buf[pos++] = (v & 0xff);
        buf[pos++] = (v>>8)&0xff;
        buf[pos++] = (v>>16)&0xff;
        buf[pos++] = (v>>24)&0xff;
    }

    return total_size;
}

static inline zint32_t __int32_t_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        p[element] = (((zuint32_t)buf[pos+3])<<24) +
                      (((zuint32_t)buf[pos+2])<<16) +
                      (((zuint32_t)buf[pos+1])<<8) +
                       ((zuint32_t)buf[pos+0]);
        pos+=4;
    }

    return total_size;
}

static inline zuint32_t __int32_t_clone_array(const zint32_t *p, zint32_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zint32_t);
    memcpy(q, p, n);
    return n;
}

/**
 * INT64_T
 */
#define __int64_t_hash_recursive(p) 0
#define __int64_t_decode_array_cleanup(p, sz) {}

static inline zuint32_t __int64_t_encoded_array_size(const zint64_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __int64_t_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t v = (zuint64_t) p[element];
        buf[pos++] = (v>>56)&0xff;
        buf[pos++] = (v>>48)&0xff;
        buf[pos++] = (v>>40)&0xff;
        buf[pos++] = (v>>32)&0xff;
        buf[pos++] = (v>>24)&0xff;
        buf[pos++] = (v>>16)&0xff;
        buf[pos++] = (v>>8)&0xff;
        buf[pos++] = (v & 0xff);
    }

    return total_size;
}

static inline zint32_t __int64_t_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t a = (((zuint32_t)buf[pos+0])<<24) +
                     (((zuint32_t)buf[pos+1])<<16) +
                     (((zuint32_t)buf[pos+2])<<8) +
                      ((zuint32_t)buf[pos+3]);
        pos+=4;
        zuint64_t b = (((zuint32_t)buf[pos+0])<<24) +
                     (((zuint32_t)buf[pos+1])<<16) +
                     (((zuint32_t)buf[pos+2])<<8) +
                      ((zuint32_t)buf[pos+3]);
        pos+=4;
        p[element] = (a<<32) + (b&0xffffffff);
    }

    return total_size;
}

static inline zint32_t __int64_t_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zint64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t v = (zuint64_t) p[element];
        buf[pos++] = (v & 0xff);
        buf[pos++] = (v>>8)&0xff;
        buf[pos++] = (v>>16)&0xff;
        buf[pos++] = (v>>24)&0xff;
        buf[pos++] = (v>>32)&0xff;
        buf[pos++] = (v>>40)&0xff;
        buf[pos++] = (v>>48)&0xff;
        buf[pos++] = (v>>56)&0xff;
    }

    return total_size;
}

static inline zint32_t __int64_t_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zint64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_INT64_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t b = (((zuint32_t)buf[pos+3])<<24) +
                     (((zuint32_t)buf[pos+2])<<16) +
                     (((zuint32_t)buf[pos+1])<<8) +
                      ((zuint32_t)buf[pos+0]);
        pos+=4;
        zuint64_t a = (((zuint32_t)buf[pos+3])<<24) +
                     (((zuint32_t)buf[pos+2])<<16) +
                     (((zuint32_t)buf[pos+1])<<8) +
                      ((zuint32_t)buf[pos+0]);
        pos+=4;
        p[element] = (a<<32) + (b&0xffffffff);
    }

    return total_size;
}

static inline zuint32_t __int64_t_clone_array(const zint64_t *p, zint64_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zint64_t);
    memcpy(q, p, n);
    return n;
}

/**
 * FLOAT
 */
typedef union __cm__float_zuint32_t {
    zfloat32_t zflt32;
    zuint32_t zuint32;
} __cm__float_zuint32_t;

#define __float_hash_recursive(p) 0
#define __float_decode_array_cleanup(p, sz) {}

static inline zuint32_t __float_encoded_array_size(const zfloat32_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __float_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zfloat32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__float_zuint32_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zflt32 = p[element];
        buf[pos++] = (tmp.zuint32 >> 24) & 0xff;
        buf[pos++] = (tmp.zuint32 >> 16) & 0xff;
        buf[pos++] = (tmp.zuint32 >>  8) & 0xff;
        buf[pos++] = (tmp.zuint32      ) & 0xff;
    }

    return total_size;
}

static inline zint32_t __float_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zfloat32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__float_zuint32_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zuint32 = (((zuint32_t)buf[pos + 0]) << 24) |
                      (((zuint32_t)buf[pos + 1]) << 16) |
                      (((zuint32_t)buf[pos + 2]) <<  8) |
                       ((zuint32_t)buf[pos + 3]);
        p[element] = tmp.zflt32;
        pos += 4;
    }

    return total_size;
}

static inline zint32_t __float_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zfloat32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__float_zuint32_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zflt32 = p[element];
        buf[pos++] = (tmp.zuint32      ) & 0xff;
        buf[pos++] = (tmp.zuint32 >>  8) & 0xff;
        buf[pos++] = (tmp.zuint32 >> 16) & 0xff;
        buf[pos++] = (tmp.zuint32 >> 24) & 0xff;
    }

    return total_size;
}

static inline zint32_t __float_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zfloat32_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_FLOAT_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__float_zuint32_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zuint32 = (((zuint32_t)buf[pos + 3]) << 24) |
                      (((zuint32_t)buf[pos + 2]) << 16) |
                      (((zuint32_t)buf[pos + 1]) <<  8) |
                       ((zuint32_t)buf[pos + 0]);
        p[element] = tmp.zflt32;
        pos += 4;
    }

    return total_size;
}

static inline zuint32_t __float_clone_array(const zfloat32_t *p, zfloat32_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zfloat32_t);
    memcpy(q, p, n);
    return n;
}

/**
 * DOUBLE
 */
typedef union __cm__double_zuint64_t {
    zfloat64_t zfloat64;
    zuint64_t zuint64;
} __cm__double_zuint64_t;

#define __double_hash_recursive(p) 0
#define __double_decode_array_cleanup(p, sz) {}

static inline zuint32_t __double_encoded_array_size(const zfloat64_t *p, zuint32_t elements)
{
    (void)p;
    return ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS * elements;
}

static inline zint32_t __double_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zfloat64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__double_zuint64_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zfloat64 = p[element];
        buf[pos++] = (tmp.zuint64 >> 56) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 48) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 40) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 32) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 24) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 16) & 0xff;
        buf[pos++] = (tmp.zuint64 >>  8) & 0xff;
        buf[pos++] = (tmp.zuint64      ) & 0xff;
    }

    return total_size;
}

static inline zint32_t __double_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zfloat64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__double_zuint64_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t a = (((zuint32_t) buf[pos + 0]) << 24) +
                      (((zuint32_t) buf[pos + 1]) << 16) +
                      (((zuint32_t) buf[pos + 2]) <<  8) +
                       ((zuint32_t) buf[pos + 3]);
        pos += 4;
        zuint64_t b = (((zuint32_t) buf[pos + 0]) << 24) +
                      (((zuint32_t) buf[pos + 1]) << 16) +
                      (((zuint32_t) buf[pos + 2]) <<  8) +
                       ((zuint32_t) buf[pos + 3]);
        pos += 4;
        tmp.zuint64 = (a << 32) + (b & 0xffffffff);
        p[element] = tmp.zfloat64;
    }

    return total_size;
}

static inline zint32_t __double_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, const zfloat64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__double_zuint64_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        tmp.zfloat64 = p[element];
        buf[pos++] = (tmp.zuint64      ) & 0xff;
        buf[pos++] = (tmp.zuint64 >>  8) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 16) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 24) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 32) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 40) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 48) & 0xff;
        buf[pos++] = (tmp.zuint64 >> 56) & 0xff;
    }

    return total_size;
}

static inline zint32_t __double_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zfloat64_t *p, zuint32_t elements)
{
    zuint32_t total_size = ZCM_CORETYPES_DOUBLE_NUM_BYTES_ON_BUS * elements;
    zuint8_t *buf = (zuint8_t*) _buf;
    zuint32_t pos = offset;
    zuint32_t element;
    __cm__double_zuint64_t tmp;

    if (maxlen < total_size) return -1;

    for (element = 0; element < elements; ++element) {
        zuint64_t b = (((zuint32_t)buf[pos + 3]) << 24) +
                      (((zuint32_t)buf[pos + 2]) << 16) +
                      (((zuint32_t)buf[pos + 1]) <<  8) +
                       ((zuint32_t)buf[pos + 0]);
        pos += 4;
        zuint64_t a = (((zuint32_t)buf[pos + 3]) << 24) +
                      (((zuint32_t)buf[pos + 2]) << 16) +
                      (((zuint32_t)buf[pos + 1]) <<  8) +
                       ((zuint32_t)buf[pos + 0]);
        pos += 4;
        tmp.zuint64 = (a << 32) + (b & 0xffffffff);
        p[element] = tmp.zfloat64;
    }

    return total_size;
}

static inline zuint32_t __double_clone_array(const zfloat64_t *p, zfloat64_t *q, zuint32_t elements)
{
    zuint32_t n = elements * sizeof(zfloat64_t);
    memcpy(q, p, n);
    return n;
}

/**
 * STRING
 */
#define __string_hash_recursive(p) 0

static inline void __string_decode_array_cleanup(char **s, zuint32_t elements)
{
    zuint32_t element;
    for (element = 0; element < elements; ++element)
        zcm_free(s[element]);
}

static inline zuint32_t __string_encoded_array_size(zchar_t * const *s, zuint32_t elements)
{
    zuint32_t size = 0;

    zuint32_t element;
    for (element = 0; element < elements; ++element) {
        size += ZCM_CORETYPES_INT32_NUM_BYTES_ON_BUS + // an int32_t for encoding length
                strlen(s[element]) +
                ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS;   // an int8_t null character
    }

    return size;
}

static inline zint32_t __string_encode_array(void *_buf, zuint32_t offset, zuint32_t maxlen, zchar_t * const *p, zuint32_t elements)
{
    zuint32_t pos = 0;
    zuint32_t element;

    for (element = 0; element < elements; ++element) {
        zint32_t length = strlen(p[element]) + ZCM_CORETYPES_INT8_NUM_BYTES_ON_BUS; // length includes \0
        zint32_t thislen;

        thislen = __int32_t_encode_array(_buf, offset + pos, maxlen - pos, &length, 1);
        if (thislen < 0) return thislen; else pos += thislen;

        thislen = __int8_t_encode_array(_buf, offset + pos, maxlen - pos, (int8_t*) p[element], length);
        if (thislen < 0) return thislen; else pos += thislen;
    }

    return pos;
}

static inline zint32_t __string_decode_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zchar_t **p, zuint32_t elements)
{
    zuint32_t pos = 0;
    zuint32_t element;

    for (element = 0; element < elements; ++element) {
        zint32_t length;
        zint32_t thislen;

        // read length including \0
        thislen = __int32_t_decode_array(_buf, offset + pos, maxlen - pos, &length, 1);
        if (thislen < 0) return thislen; else pos += thislen;

        p[element] = (zchar_t*) zcm_malloc(sizeof(zchar_t) * length);
        thislen = __int8_t_decode_array(_buf, offset + pos, maxlen - pos, (zint8_t*) p[element], length);
        if (thislen < 0) return thislen; else pos += thislen;
    }

    return pos;
}

static inline zint32_t __string_encode_little_endian_array(void *_buf, zuint32_t offset, zuint32_t maxlen, zchar_t * const *p, zuint32_t elements)
{
    zuint32_t pos = 0;
    zuint32_t element;

    for (element = 0; element < elements; ++element) {
        zint32_t length = strlen(p[element]) + 1; // length includes \0
        zint32_t thislen;

        thislen = __int32_t_encode_little_endian_array(_buf, offset + pos, maxlen - pos, &length, 1);
        if (thislen < 0) return thislen; else pos += thislen;

        thislen = __int8_t_encode_little_endian_array(_buf, offset + pos, maxlen - pos, (zint8_t*) p[element], length);
        if (thislen < 0) return thislen; else pos += thislen;
    }

    return pos;
}

static inline zint32_t __string_decode_little_endian_array(const void *_buf, zuint32_t offset, zuint32_t maxlen, zchar_t **p, zuint32_t elements)
{
    zuint32_t pos = 0;
    zuint32_t element;

    for (element = 0; element < elements; ++element) {
        zint32_t length;
        zint32_t thislen;

        // read length including \0
        thislen = __int32_t_decode_little_endian_array(_buf, offset + pos, maxlen - pos, &length, 1);
        if (thislen < 0) return thislen; else pos += thislen;

        p[element] = (zchar_t*) zcm_malloc(sizeof(zchar_t) * length);
        thislen = __int8_t_decode_little_endian_array(_buf, offset + pos, maxlen - pos, (zint8_t*) p[element], length);
        if (thislen < 0) return thislen; else pos += thislen;
    }

    return pos;
}

static inline zuint32_t __string_clone_array(zchar_t * const *p, zchar_t **q, zuint32_t elements)
{
    zuint32_t ret = 0;
    zuint32_t element;
    for (element = 0; element < elements; ++element) {
        // because strdup is not C99
        zuint32_t len = strlen(p[element]) + 1; // length includes \0
        ret += len;
        q[element] = (zchar_t*) zcm_malloc(sizeof(zchar_t) * len);
        memcpy(q[element], p[element], len);
    }
    return ret;
}

/**
 * Describes the type of a single field in an ZCM message.
 */
typedef enum {
    ZCM_FIELD_INT8_T,
    ZCM_FIELD_INT16_T,
    ZCM_FIELD_INT32_T,
    ZCM_FIELD_INT64_T,
    ZCM_FIELD_BYTE,
    ZCM_FIELD_FLOAT,
    ZCM_FIELD_DOUBLE,
    ZCM_FIELD_STRING,
    ZCM_FIELD_BOOLEAN,
    ZCM_FIELD_USER_TYPE
} zcm_field_type_t;

#define ZCM_TYPE_FIELD_MAX_DIM 50

/**
 * Describes a single zcmtype field's datatype and array dimmensions
 */
typedef struct _zcm_field_t zcm_field_t;
struct _zcm_field_t
{
    /**
     * name of the field
     */
    const zchar_t *name;

    /**
     * datatype of the field
     **/
    zcm_field_type_t type;

    /**
     * datatype of the field (in string format)
     * this should be the same as in the zcm type decription file
     */
    const zchar_t *typestr;

    /**
     * number of array dimensions
     * if the field is scalar, num_dim should equal 0
     */
    zint32_t num_dim;

    /**
     * the size of each dimension. Valid on [0:num_dim-1].
     */
    zint32_t dim_size[ZCM_TYPE_FIELD_MAX_DIM];

    /**
     * a boolean describing whether the dimension is
     * variable. Valid on [0:num_dim-1].
     */
    zint8_t  dim_is_variable[ZCM_TYPE_FIELD_MAX_DIM];

    /**
     * a data pointer to the start of this field
     */
    void *data;
};

typedef zint32_t    (*zcm_encode_t)(void *buf, zuint32_t offset, zuint32_t maxlen, const void *p);
typedef zint32_t    (*zcm_decode_t)(const void *buf, zuint32_t offset, zuint32_t maxlen, void *p);
typedef zint32_t    (*zcm_decode_cleanup_t)(void *p);
typedef zuint32_t   (*zcm_encoded_size_t)(const void *p);
typedef zuint32_t   (*zcm_struct_size_t)(void);
typedef zuint32_t   (*zcm_num_fields_t)(void);
typedef zint32_t    (*zcm_get_field_t)(const void *p, zuint32_t i, zcm_field_t *f);
typedef zint64_t    (*zcm_get_hash_t)(void);

/**
 * Describes an zcmtype info, enabling introspection
 */
typedef struct _zcm_type_info_t zcm_type_info_t;
struct _zcm_type_info_t
{
    zcm_encode_t          encode;
    zcm_decode_t          decode;
    zcm_decode_cleanup_t  decode_cleanup;
    zcm_encoded_size_t    encoded_size;
    zcm_struct_size_t     struct_size;
    zcm_num_fields_t      num_fields;
    zcm_get_field_t       get_field;
    zcm_get_hash_t        get_hash;

};

#ifdef __cplusplus
}
#endif

#endif // _ZCM_LIB_INLINE_H
