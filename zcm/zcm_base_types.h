#ifndef __ZCM_BASE_TYPES__
#define __ZCM_BASE_TYPES__

#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <string.h>

#ifndef ZCM_EMBEDDED
#include <assert.h>
#define ZCM_ASSERT(X) assert(X)
#else
#define ZCM_ASSERT(X)
#endif

#define ZCM_BASE_TYPES \
    X( uint8_t,    zbyte_t) /* Must be at least  8 bits long */ \
    X(  int8_t,    zbool_t) /* Must be at least  8 bits long */ \
    /* zchar_t must be compatible with all str functions in string.h */ \
    X(    char,    zchar_t) /* Must be at least  8 bits long */ \
    X( uint8_t,   zuint8_t) /* Must be at least  8 bits long */ \
    X(  int8_t,    zint8_t) /* Must be at least  8 bits long */ \
    X(uint16_t,  zuint16_t) /* Must be at least 16 bits long */ \
    X( int16_t,   zint16_t) /* Must be at least 16 bits long */ \
    X(uint32_t,  zuint32_t) /* Must be at least 32 bits long */ \
    X( int32_t,   zint32_t) /* Must be at least 32 bits long */ \
    X(uint64_t,  zuint64_t) /* Must be at least 64 bits long */ \
    X( int64_t,   zint64_t) /* Must be at least 64 bits long */ \
    X(   float, zfloat32_t) /* Must be at least 32 bits long */ \
    X(  double, zfloat64_t) /* Must be at least 64 bits long */ \
    X(  size_t,    zsize_t) /* Must have a size no less than the datatype returned by sizeof */ \
    X(     int,     zint_t) \
    X(unsigned,    zuint_t) \

#ifdef __cplusplus
#include <string>
#define ZCM_BASE_TYPES_CPP \
    X(std::string, zstring_t) /* Must be a string of zchar_t */
#else
#define ZCM_BASE_TYPES_CPP
#endif

#ifndef ZCM_EMBEDDED
#include <sys/types.h>
#define ZCM_BASE_TYPES_NO_EMBEDDED \
    X( ssize_t,   zssize_t) /* Must have a size equal to zsize_t */ \
    X(   off_t,     zoff_t)
#else
#define ZCM_BASE_TYPES_NO_EMBEDDED
#endif

#define ZCM_RETURN_CODES \
    X(ZCM_EOK,              0, "Okay, no errors"                       ) \
    X(ZCM_EINVALID,         1, "Invalid arguments"                     ) \
    X(ZCM_EAGAIN  ,         2, "Resource unavailable, try again"       ) \
    X(ZCM_ECONNECT,         3, "Transport connection failed"           ) \
    X(ZCM_EINTR   ,         4, "Operation was unexpectedly interrupted") \
    X(ZCM_EUNKNOWN,         5, "Unknown error"                         ) \
    X(ZCM_EMEMORY,          6, "Out of memory"                         ) \
    X(ZCM_EOF,              7, "End of file"                           ) \
    X(ZCM_NUM_RETURN_CODES, 8, "Invalid return code"                   )

#define ztrue  (1)
#define zfalse (0)

#define ZCM_CHANNEL_MAXLEN 32

#define zcm_malloc(sz) ((sz) ? malloc(sz) : NULL)
#define zcm_calloc(sz,elts) ((sz) ? ((elts) ? calloc(sz, elts) : NULL) : NULL)
#define zcm_free free

#define X(NATIVE_TYPE, ZCM_BASE_TYPE) \
    typedef NATIVE_TYPE ZCM_BASE_TYPE;
ZCM_BASE_TYPES
ZCM_BASE_TYPES_CPP
ZCM_BASE_TYPES_NO_EMBEDDED
#undef X

/* Return codes */
enum zcm_return_codes {
    #define X(n, v, s) n = v,
    ZCM_RETURN_CODES
    #undef X
};
typedef enum zcm_return_codes zcm_retcode_t;

#endif /* __ZCM_BASE_TYPES__ */
