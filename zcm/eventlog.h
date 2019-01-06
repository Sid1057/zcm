#ifndef _ZCM_EVENTLOG_H
#define _ZCM_EVENTLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include "zcm/zcm_base_types.h"

/* Note: some language bindings depend on the specific memory layout
 *       of ZCM structures. If you change these, be sure to update
 *       language bindings to match. */

typedef struct _zcm_eventlog_event_t zcm_eventlog_event_t;
struct _zcm_eventlog_event_t
{
    zuint64_t  eventnum;   /* populated by write_event */
    zuint64_t  timestamp;
    zuint32_t  channellen;
    zuint32_t  datalen;
    zchar_t*  channel;
    zuint8_t* data;
};

typedef struct _zcm_eventlog_t zcm_eventlog_t;
struct _zcm_eventlog_t
{
    FILE* f;
    zint64_t eventcount;
};

/**** Methods for creation/deletion ****/
zcm_eventlog_t* zcm_eventlog_create(const zchar_t* path, const zchar_t* mode);
void zcm_eventlog_destroy(zcm_eventlog_t* eventlog);


/**** Methods for general operations ****/
FILE* zcm_eventlog_get_fileptr(zcm_eventlog_t* eventlog);
zcm_retcode_t zcm_eventlog_seek_to_timestamp(zcm_eventlog_t* eventlog, zuint64_t ts);


/**** Methods for read/write ****/
// NOTE: The returned zcm_eventlog_event_t must be freed by zcm_eventlog_free_event()
zcm_eventlog_event_t* zcm_eventlog_read_next_event(zcm_eventlog_t* eventlog);
zcm_eventlog_event_t* zcm_eventlog_read_prev_event(zcm_eventlog_t* eventlog);
zcm_eventlog_event_t* zcm_eventlog_read_event_at_offset(zcm_eventlog_t* eventlog, zoff_t offset);
void zcm_eventlog_free_event(zcm_eventlog_event_t* event);
zcm_retcode_t zcm_eventlog_write_event(zcm_eventlog_t* eventlog, const zcm_eventlog_event_t* event);


#ifdef __cplusplus
}
#endif

#endif /* _ZCM_EVENTLOG_H */
