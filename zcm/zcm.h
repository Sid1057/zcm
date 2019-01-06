#ifndef _ZCM_H
#define _ZCM_H

#include "zcm_base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ZCM_EMBEDDED
#include "eventlog.h"
#endif

enum zcm_type {
    ZCM_BLOCKING,
    ZCM_NONBLOCKING
};

/* Forward typedef'd structs */
typedef struct zcm_trans_t    zcm_trans_t;
typedef struct zcm_t          zcm_t;
typedef struct zcm_recv_buf_t zcm_recv_buf_t;
typedef struct zcm_sub_t      zcm_sub_t;

/* Generic message handler function type */
typedef void (*zcm_msg_handler_t)(const zcm_recv_buf_t* rbuf,
                                  const zchar_t* channel, void* usr);

/* Note: some language bindings depend on the specific memory layout
 *       of ZCM structures. If you change these, be sure to update
 *       language bindings to match. */

/* Primary ZCM object that handles all top-level interactions including
   delegation between blocking and non-blocking interfaces */
struct zcm_t
{
    enum zcm_type         type;
    void*                 impl;
    enum zcm_return_codes err; /* the last error code */
};

/* ZCM Receive buffer for one message */
struct zcm_recv_buf_t
{
    zuint64_t recv_utime;
    zcm_t*    zcm;
    zuint8_t* data; /* NOTE: do not free, the library manages this memory */
    zuint32_t data_size;
};

#ifndef ZCM_EMBEDDED
enum zcm_return_codes zcm_retcode_name_to_enum(const zchar_t* zcm_retcode_name);
#endif

/* Standard create/destroy functions. These will malloc() and free() the zcm_t object.
   Sets zcm errno on failure */
#ifndef ZCM_EMBEDDED
zcm_t* zcm_create(const zchar_t* url);
#endif
zcm_t* zcm_create_trans(zcm_trans_t* zt);
void   zcm_destroy(zcm_t* zcm);

#ifndef ZCM_EMBEDDED
/* Initialize a zcm object allocated by caller */
zcm_retcode_t zcm_init(zcm_t* zcm, const zchar_t* url);
#endif

/* Initialize a zcm instance allocated by caller using a transport provided by caller */
zcm_retcode_t zcm_init_trans(zcm_t* zcm, zcm_trans_t* zt);

/* Cleanup a zcm object allocated by caller */
void zcm_cleanup(zcm_t* zcm);

/* Return the last error: a valid from enum zcm_return_codes */
enum zcm_return_codes zcm_errno(const zcm_t* zcm);

/* Return the last error in string format */
const zchar_t* zcm_strerror(const zcm_t* zcm);

/* Returns the error string from the error number */
const zchar_t* zcm_strerrno(enum zcm_return_codes err);

/* Subscribe to zcm messages
   Returns a subscription object on success, and NULL on failure
   Sets zcm errno on failure */
zcm_sub_t* zcm_subscribe(zcm_t* zcm, const zchar_t* channel, zcm_msg_handler_t cb, void* usr);

/* Subscribe to zcm messages
   Returns a subscription object on success, and NULL on failure.
   Can fail to subscribe if zcm is already running
   Sets zcm errno on failure */
zcm_sub_t* zcm_try_subscribe(zcm_t* zcm, const zchar_t* channel, zcm_msg_handler_t cb, void* usr);

/* Unsubscribe to zcm messages, freeing the subscription object
   Returns ZCM_EOK on success, error code on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_unsubscribe(zcm_t* zcm, zcm_sub_t* sub);

/* Unsubscribe to zcm messages, freeing the subscription object
   Returns ZCM_EOK on success, error code on failure
   Can fail to subscribe if zcm is already running
   Sets zcm errno on failure */
zcm_retcode_t zcm_try_unsubscribe(zcm_t* zcm, zcm_sub_t* sub);

/* Publish a zcm message buffer. Note: the message may not be completely
   sent after this call has returned. To block until the messages are transmitted,
   call the zcm_flush() method.
   Returns true on success, and false on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_publish(zcm_t* zcm, const zchar_t* channel, const zuint8_t* data, zuint32_t len);

/* Block until all published messages have been sent even if the underlying
   transport is nonblocking. Additionally, dispatches all messages that have
   already been received sequentially in this thread. */
void zcm_flush(zcm_t* zcm);

/* Nonblocking version of flush as defined above.
   If you want to guarantee that this function succeeds at some point,
   you should zcm_pause() first.
   Returns true on success, and false on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_try_flush(zcm_t* zcm);

#ifndef ZCM_EMBEDDED
/* Blocking Mode Only: Functions for controlling the message dispatch loop */
void zcm_run(zcm_t* zcm);
void zcm_start(zcm_t* zcm);
void zcm_stop(zcm_t* zcm);
/* Returns true on success, and false on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_try_stop(zcm_t* zcm);
void zcm_pause(zcm_t* zcm); /* pauses message dispatch and publishing, not transport */
void zcm_resume(zcm_t* zcm);
/* Returns true on success, and false on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_handle(zcm_t* zcm);
/* Determines how many messages can be stored from the transport without being dispatched
   As well as the number of messages that may be stored from the user without being
   transmitted by the transport. Normal operation does not require the user to modify
   this, but if the user is using zcm_pause() and forcing dispatches/transmission through
   calls to zcm_flush(), it will be important to set an appropriate queue size based on
   traffic and flush frequency. Note that if either queue reaches maximum capacity,
   messages will not be read from / sent to the transport, which could cause significant
   issues depending on the transport. */
void zcm_set_queue_size(zcm_t* zcm, zuint32_t numMsgs);
/* Returns true on success, and false on failure
   Sets zcm errno on failure */
zcm_retcode_t zcm_try_set_queue_size(zcm_t* zcm, zuint32_t numMsgs);
#endif

/* Non-Blocking Mode Only: Functions checking and dispatching messages
   Returns true on success, and false on failure
   Sets zcm errno on failure
   zcm errno will be ZCM_EAGAIN if no messages, error code otherwise */
zcm_retcode_t zcm_handle_nonblock(zcm_t* zcm);

/*
 * Version: M.m.u
 *   M: Major
 *   m: Minor
 *   u: Micro
 */
#define ZCM_MAJOR_VERSION 1
#define ZCM_MINOR_VERSION 0
#define ZCM_MICRO_VERSION 0

#ifdef __cplusplus
}
#endif

#endif /* _ZCM_H */
