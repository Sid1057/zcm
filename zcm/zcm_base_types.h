#ifndef __ZCM_BASE_TYPES__
#define __ZCM_BASE_TYPES__ \
X( uint8_t,   zuint8_t) \
X(  int8_t,    zint8_t) \
X(uint16_t,  zuint16_t) \
X( int16_t,   zint16_t) \
X(uint32_t,  zuint32_t) \
X( int32_t,   zint32_t) \
X(uint64_t,  zuint64_t) \
X( int64_t,   zint64_t) \
X(   float, zfloat32_t) \
X(  double, zfloat64_t) \

#include <stdint.h>
#include <float.h>

#define X(NATIVE_TYPE, ZCM_BASE_TYPE) \
    typedef NATIVE_TYPE ZCM_BASE_TYPE
#undef X

#endif // __ZCM_BASE_TYPES__
