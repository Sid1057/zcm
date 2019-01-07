#ifndef _ZCM_NONBLOCKING_H
#define _ZCM_NONBLOCKING_H

#include "zcm/zcm.h"
#include "zcm/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zcm_nonblocking zcm_nonblocking_t;

zcm_nonblocking_t* zcm_nonblocking_create(zcm_t* z, zcm_trans_t* trans);
void               zcm_nonblocking_destroy(zcm_nonblocking_t* zcm);

zcm_retcode_t zcm_nonblocking_publish(zcm_nonblocking_t* zcm, const zchar_t* channel,
                                      const zuint8_t* data, zuint32_t len);

zcm_sub_t*    zcm_nonblocking_subscribe(zcm_nonblocking_t* zcm, const zchar_t* channel,
                                        zcm_msg_handler_t cb, void* usr);

zcm_retcode_t zcm_nonblocking_unsubscribe(zcm_nonblocking_t* zcm, zcm_sub_t* sub);

zcm_retcode_t zcm_nonblocking_handle(zcm_nonblocking_t* zcm);

void zcm_nonblocking_flush(zcm_nonblocking_t* zcm);

#ifdef __cplusplus
}
#endif

#endif /* _ZCM_NONBLOCKING_H */
