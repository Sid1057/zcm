#ifndef _ZCM_BLOCKING_H
#define _ZCM_BLOCKING_H

#include "zcm/zcm.h"
#include "zcm/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  zcm_blocking zcm_blocking_t;
zcm_blocking_t* zcm_blocking_create(zcm_t* z, zcm_trans_t* trans);
void            zcm_blocking_destroy(zcm_blocking_t* zcm);

zcm_retcode_t zcm_blocking_publish(zcm_blocking_t* zcm, const zchar_t* channel,
                                   const zuint8_t* data, zuint32_t len);

zcm_sub_t* zcm_blocking_subscribe(zcm_blocking_t* zcm, const zchar_t* channel,
                                  zcm_msg_handler_t cb, void* usr);
zcm_sub_t* zcm_blocking_try_subscribe(zcm_blocking_t* zcm, const zchar_t* channel,
                                      zcm_msg_handler_t cb, void* usr);

zcm_retcode_t zcm_blocking_unsubscribe(zcm_blocking_t* zcm, zcm_sub_t* sub);
zcm_retcode_t zcm_blocking_try_unsubscribe(zcm_blocking_t* zcm, zcm_sub_t* sub);

void zcm_blocking_flush(zcm_blocking_t* zcm);
zcm_retcode_t zcm_blocking_try_flush(zcm_blocking_t* zcm);

void          zcm_blocking_run(zcm_blocking_t* zcm);
void          zcm_blocking_start(zcm_blocking_t* zcm);
zcm_retcode_t zcm_blocking_try_stop(zcm_blocking_t* zcm);
void          zcm_blocking_stop(zcm_blocking_t* zcm);
void          zcm_blocking_pause(zcm_blocking_t* zcm);
void          zcm_blocking_resume(zcm_blocking_t* zcm);
zcm_retcode_t zcm_blocking_handle(zcm_blocking_t* zcm);
void          zcm_blocking_set_queue_size(zcm_blocking_t* zcm, zuint32_t numMsgs);
zcm_retcode_t zcm_blocking_try_set_queue_size(zcm_blocking_t* zcm, zuint32_t numMsgs);

#ifdef __cplusplus
}
#endif

#endif /* _ZCM_BLOCKING_H */
