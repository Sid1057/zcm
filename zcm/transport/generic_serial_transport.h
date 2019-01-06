#ifndef _ZCM_TRANS_NONBLOCKING_SERIAL_H
#define _ZCM_TRANS_NONBLOCKING_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "zcm/zcm.h"
#include "zcm/transport.h"

zcm_trans_t *zcm_trans_generic_serial_create(
        zuint32_t (*get)(zuint8_t* data, zuint32_t nData, void* usr),
        zuint32_t (*put)(const zuint8_t* data, zuint32_t nData, void* usr),
        void* put_get_usr,
        zuint64_t (*timestamp_now)(void* usr),
        void* time_usr,
        zuint32_t MTU, zuint32_t bufSize);

// frees all resources inside of zt and frees zt itself
void zcm_trans_generic_serial_destroy(zcm_trans_t* zt);

zcm_retcode_t serial_update_rx(zcm_trans_t *zt);
zcm_retcode_t serial_update_tx(zcm_trans_t *zt);

#ifdef __cplusplus
}
#endif

#endif /* _ZCM_TRANS_NONBLOCKING_SERIAL_H */
