#ifndef __ZCM_BASE_TYPES__
#define __ZCM_BASE_TYPES__ \
X(1,  uint8_t,   zuint8_t) \
X(1,   int8_t,    zint8_t) \
X(2, uint16_t,  zuint16_t) \
X(2,  int16_t,   zint16_t) \
X(4, uint32_t,  zuint32_t) \
X(4,  int32_t,   zint32_t) \
X(8, uint64_t,  zuint64_t) \
X(8,  int64_t,   zint64_t) \
X(4,    float, zfloat32_t) \
X(8,   double, zfloat64_t) \

#include <stdint.h>
#include <float.h>

#define X(_, NATIVE_TYPE, ZCM_BASE_TYPE) \
    typedef NATIVE_TYPE ZCM_BASE_TYPE
#undef X

#endif // __ZCM_BASE_TYPES__
