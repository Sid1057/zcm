#include "zcm/zcm.h"
#include "zcm/transport.h"
#include "generic_serial_transport.h"

#ifndef ZCM_GENERIC_SERIAL_ESCAPE_CHAR
#define ZCM_GENERIC_SERIAL_ESCAPE_CHAR (0xcc)
#endif

// Framing (size = 9 + chan_len + data_len)
//   0xCC
//   0x00
//   chan_len
//   data_len  (4 bytes)
//   *chan
//   *data
//   sum1(*chan, *data)
//   sum2(*chan, *data)
#define FRAME_BYTES 9

// Note: there is little to no error checking in this, misuse will cause problems
typedef struct circBuffer_t circBuffer_t;
struct circBuffer_t
{
    zuint8_t* data;
    zuint32_t capacity;
    zuint32_t front;
    zuint32_t back;
};

zbool_t cb_init(circBuffer_t* cb, zuint32_t sz)
{
    cb->capacity = sz;
    cb->front = 0;
    cb->back  = 0;
    if (cb->capacity == 0) return false;
    cb->data = zcm_malloc(cb->capacity * sizeof(zuint8_t));
    if (cb->data == NULL) {
        cb->capacity = 0;
        return false;
    }
    return true;
}

void cb_deinit(circBuffer_t* cb)
{
    zcm_free(cb->data);
    cb->data = NULL;
    cb->capacity = 0;
}

zuint32_t cb_size(circBuffer_t* cb)
{
    if (cb->back >= cb->front) return cb->back - cb->front;
    else                       return cb->capacity - (cb->front - cb->back);
}

zuint32_t cb_room(circBuffer_t* cb)
{
    return cb->capacity - 1 - cb_size(cb);
}

void cb_push(circBuffer_t* cb, zuint8_t d)
{
    ZCM_ASSERT((cb->capacity > cb_size(cb) + 1) && "cb_push 1");
    ZCM_ASSERT((cb_room(cb) > 0) && "cb_push 2");
    cb->data[cb->back++] = d;
    ZCM_ASSERT((cb->back <= cb->capacity) && "cb_push 3");
    if (cb->back == cb->capacity) cb->back = 0;
}

zuint8_t cb_top(circBuffer_t* cb, zuint32_t offset)
{
    ZCM_ASSERT((cb_size(cb) > offset) && "cb_top 1");
    zuint32_t idx = cb->front + offset;
    if (idx >= cb->capacity) idx -= cb->capacity;
    return cb->data[idx];
}

void cb_pop(circBuffer_t* cb, zuint32_t num)
{
    ZCM_ASSERT((cb_size(cb) >= num) && "cb_pop 1");
    cb->front += num;
    if (cb->front >= cb->capacity) cb->front -= cb->capacity;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
zuint32_t cb_flush_out(circBuffer_t* cb,
                      zuint32_t (*write)(const zuint8_t* data, zuint32_t num, void* usr),
                      void* usr)
{
	zuint32_t written = 0;
	zuint32_t n;
    zuint32_t sz = cb_size(cb);

    if (sz == 0) return 0;

    zuint32_t contiguous = MIN(cb->capacity - cb->front, sz);
    zuint32_t wrapped    = sz - contiguous;

    n = write(cb->data + cb->front, contiguous, usr);
    written += n;
    cb_pop(cb, n);

    // If we failed to write everything we tried to write, or if there's nothing
    // left to write, return.
    if (written != contiguous || wrapped == 0) return written;

    n = write(cb->data, wrapped, usr);
    written += n;
    cb_pop(cb, n);
    return written;
}

// NOTE: This function should never be called w/ bytes > cb_room(cb)
zuint32_t cb_flush_in(circBuffer_t* cb, zuint32_t bytes,
                     zuint32_t (*read)(zuint8_t* data, zuint32_t num, void* usr),
                     void* usr)
{
    ZCM_ASSERT((bytes <= cb_room(cb)) && "cb_flush_in 1");
	zuint32_t bytesRead = 0;
	zuint32_t n;

    // Find out how much room is left between back and end of buffer or back and front
    // of buffer. Because we already know there's room for whatever we're about to place,
    // if back < front, we can just read in every byte starting at "back".
    if (cb->back < cb->front) {
    	bytesRead += read(cb->data + cb->back, bytes, usr);
        ZCM_ASSERT((bytesRead <= bytes) && "cb_flush_in 2");
        cb->back += bytesRead;
        return bytesRead;
    }

    // Otherwise, we need to be a bit more careful about overflowing the back of the buffer.
    zuint32_t contiguous = MIN(cb->capacity - cb->back, bytes);
    zuint32_t wrapped    = bytes - contiguous;

    n = read(cb->data + cb->back, contiguous, usr);
    ZCM_ASSERT((n <= contiguous) && "cb_flush_in 3");
    bytesRead += n;
    ZCM_ASSERT((bytesRead <= bytes) && "cb_flush_in 4");
    cb->back += n;
    if (n != contiguous) return bytesRead; // back could NOT have hit BUFFER_SIZE in this case

    // may need to wrap back here (if bytes >= BUFFER_SIZE - cb->back) but not otherwise
    ZCM_ASSERT((cb->back <= cb->capacity) && "cb_flush_in 5");
    if (cb->back == cb->capacity) cb->back = 0;
    if (wrapped == 0) return bytesRead;

    n = read(cb->data, wrapped, usr);
    ZCM_ASSERT((n <= wrapped) && "cb_flush_in 6");
    bytesRead += n;
    ZCM_ASSERT((bytesRead <= bytes) && "cb_flush_in 7");
    cb->back += n;
    return bytesRead;
}
#undef MIN

static zuint16_t fletcherUpdate(zuint8_t b, zuint16_t prevSum)
{
    zuint16_t sumHigh = (prevSum >> 8) & 0xff;
    zuint16_t sumLow  =  prevSum       & 0xff;
    sumHigh += sumLow += b;

    sumLow  = (sumLow  & 0xff) + (sumLow  >> 8);
    sumHigh = (sumHigh & 0xff) + (sumHigh >> 8);

    // Note: double reduction to ensure no overflow after first
    sumLow  = (sumLow  & 0xff) + (sumLow  >> 8);
    sumHigh = (sumHigh & 0xff) + (sumHigh >> 8);

    return (sumHigh << 8) | sumLow;
}

typedef struct zcm_trans_generic_serial_t zcm_trans_generic_serial_t;
struct zcm_trans_generic_serial_t
{
    zcm_trans_t trans; // This must be first to preserve pointer casting

    circBuffer_t sendBuffer;
    circBuffer_t recvBuffer;
    zuint8_t     recvChanName[ZCM_CHANNEL_MAXLEN + 1];
    zuint32_t     mtu;
    zuint8_t*    recvMsgData;

    zuint32_t (*get)(zuint8_t* data, zuint32_t nData, void* usr);
    zuint32_t (*put)(const zuint8_t* data, zuint32_t nData, void* usr);
    void* put_get_usr;

    zuint64_t (*time)(void* usr);
    void* time_usr;
};

static zcm_trans_generic_serial_t *cast(zcm_trans_t *zt);

zuint32_t serial_get_mtu(zcm_trans_generic_serial_t *zt)
{ return zt->mtu; }

zcm_retcode_t serial_sendmsg(zcm_trans_generic_serial_t *zt, zcm_msg_t msg)
{
    zuint32_t chan_len = strlen(msg.channel);
    zuint32_t nPushed = 0;

    if (chan_len > ZCM_CHANNEL_MAXLEN)                               return ZCM_EINVALID;
    if (msg.len > zt->mtu)                                           return ZCM_EINVALID;
    if (FRAME_BYTES + chan_len + msg.len > cb_room(&zt->sendBuffer)) return ZCM_EAGAIN;

    cb_push(&zt->sendBuffer, ZCM_GENERIC_SERIAL_ESCAPE_CHAR); ++nPushed;
    cb_push(&zt->sendBuffer, 0x00);                           ++nPushed;
    cb_push(&zt->sendBuffer, chan_len);                       ++nPushed;

    zuint32_t len = (zuint32_t)msg.len;
    cb_push(&zt->sendBuffer, (len>>24)&0xff); ++nPushed;
    cb_push(&zt->sendBuffer, (len>>16)&0xff); ++nPushed;
    cb_push(&zt->sendBuffer, (len>> 8)&0xff); ++nPushed;
    cb_push(&zt->sendBuffer, (len>> 0)&0xff); ++nPushed;

    zuint16_t checksum = 0xffff;
    zuint32_t i;
    for (i = 0; i < chan_len; ++i) {
        zuint8_t c = (zuint8_t) msg.channel[i];

        cb_push(&zt->sendBuffer, c); ++nPushed;

        if (c == ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
        	// the escape character doesn't count, so we have chan_len - i characters
        	// remaining in channel + the msg + the checksum.
            if (cb_room(&zt->sendBuffer) > chan_len - i + msg.len + 1) {
                cb_push(&zt->sendBuffer, c); ++nPushed;
            } else {
                cb_pop(&zt->sendBuffer, nPushed);
                return ZCM_EAGAIN;
            }
        }

        checksum = fletcherUpdate(c, checksum);
    }

    for (i = 0; i < msg.len; ++i) {
        zuint8_t c = (zuint8_t) msg.buf[i];

        cb_push(&zt->sendBuffer, c); ++nPushed;

        if (c == ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
        	// the escape character doesn't count, so we have msg.len - i characters
        	// remaining in the msg + the checksum.
            if (cb_room(&zt->sendBuffer) > msg.len - i + 1) {
                cb_push(&zt->sendBuffer, c); ++nPushed;
            } else {
                cb_pop(&zt->sendBuffer, nPushed);
                return ZCM_EAGAIN;
            }
        }

        checksum = fletcherUpdate(c, checksum);
    }

    cb_push(&zt->sendBuffer, (checksum >> 8) & 0xff); ++nPushed;
    cb_push(&zt->sendBuffer,  checksum       & 0xff); ++nPushed;

    return ZCM_EOK;
}

zcm_retcode_t serial_recvmsg_enable(zcm_trans_generic_serial_t *zt,
                                    const zchar_t *channel, zbool_t enable)
{
    // NOTE: not implemented because it is unlikely that a microprocessor is
    //       going to be hearing messages on a USB comms that it doesn't want
    //       to hear
    return ZCM_EOK;
}

zcm_retcode_t serial_recvmsg(zcm_trans_generic_serial_t *zt, zcm_msg_t *msg, zint32_t timeout)
{
    zuint64_t utime = zt->time(zt->time_usr);
    zuint32_t incomingSize = cb_size(&zt->recvBuffer);
    if (incomingSize < FRAME_BYTES) return ZCM_EAGAIN;

    zuint32_t consumed = 0;
    zuint8_t chan_len = 0;
    zuint16_t checksum = 0;
    zuint8_t expectedHighCS = 0;
    zuint8_t expectedLowCS  = 0;
    zuint16_t receivedCS = 0;

    // Sync
    if (cb_top(&zt->recvBuffer, consumed++) != ZCM_GENERIC_SERIAL_ESCAPE_CHAR) goto fail;
    if (cb_top(&zt->recvBuffer, consumed++) != 0x00)                           goto fail;

    // Msg sizes
    chan_len  = cb_top(&zt->recvBuffer, consumed++);
    msg->len  = cb_top(&zt->recvBuffer, consumed++) << 24;
    msg->len |= cb_top(&zt->recvBuffer, consumed++) << 16;
    msg->len |= cb_top(&zt->recvBuffer, consumed++) << 8;
    msg->len |= cb_top(&zt->recvBuffer, consumed++);

    if (chan_len > ZCM_CHANNEL_MAXLEN) goto fail;
    if (msg->len > zt->mtu)            goto fail;

    if (incomingSize < FRAME_BYTES + chan_len + msg->len) return ZCM_EAGAIN;

    memset(&zt->recvChanName, '\0', ZCM_CHANNEL_MAXLEN);

    checksum = 0xffff;
    zuint32_t i;
    for (i = 0; i < chan_len; ++i) {

        zuint8_t c = cb_top(&zt->recvBuffer, consumed++);

        if (c == ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
        	// the escape character doesn't count, so we have chan_len - i characters
        	// remaining in channel + the msg + the checksum.
            if (consumed + chan_len - i + msg->len + 2 > incomingSize) return ZCM_EAGAIN;

            c = cb_top(&zt->recvBuffer, consumed++);

            if (c != ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
                consumed -= 2;
                goto fail;
            }
        }

        zt->recvChanName[i] = c;
        checksum = fletcherUpdate(c, checksum);
    }

    zt->recvChanName[chan_len] = '\0';

    for (i = 0; i < msg->len; ++i) {
        if (consumed > incomingSize) return ZCM_EAGAIN;

        zuint8_t c = cb_top(&zt->recvBuffer, consumed++);

        if (c == ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
        	// the escape character doesn't count, so we have msg.len - i characters
        	// remaining in the msg + the checksum.
            if (consumed + msg->len - i + 2 > incomingSize) return ZCM_EAGAIN;

            c = cb_top(&zt->recvBuffer, consumed++);

            if (c != ZCM_GENERIC_SERIAL_ESCAPE_CHAR) {
                consumed -= 2;
                goto fail;
            }
        }

        zt->recvMsgData[i] = c;
        checksum = fletcherUpdate(c, checksum);
    }

    expectedHighCS = cb_top(&zt->recvBuffer, consumed++);
    expectedLowCS  = cb_top(&zt->recvBuffer, consumed++);
    receivedCS = (expectedHighCS << 8) | expectedLowCS;
    if (receivedCS == checksum) {
        msg->channel = (zchar_t*) zt->recvChanName;
        msg->buf     = zt->recvMsgData;
        msg->utime   = utime;
        cb_pop(&zt->recvBuffer, consumed);
        return ZCM_EOK;
    }

  fail:
    cb_pop(&zt->recvBuffer, consumed);
    // Note: because this is a nonblocking transport, timeout is ignored, so we don't need
    //       to subtract the time used here
    return serial_recvmsg(zt, msg, timeout);
}

zcm_retcode_t serial_update_rx(zcm_trans_t *_zt)
{
    zcm_trans_generic_serial_t* zt = cast(_zt);
    cb_flush_in(&zt->recvBuffer, cb_room(&zt->recvBuffer), zt->get, zt->put_get_usr);
    return ZCM_EOK;
}

zcm_retcode_t serial_update_tx(zcm_trans_t *_zt)
{
    zcm_trans_generic_serial_t* zt = cast(_zt);
    cb_flush_out(&zt->sendBuffer, zt->put, zt->put_get_usr);
    return ZCM_EOK;
}

/********************** STATICS **********************/
static zuint32_t _serial_get_mtu(zcm_trans_t *zt)
{ return serial_get_mtu(cast(zt)); }

static zcm_retcode_t _serial_sendmsg(zcm_trans_t *zt, zcm_msg_t msg)
{ return serial_sendmsg(cast(zt), msg); }

static zcm_retcode_t _serial_recvmsg_enable(zcm_trans_t *zt, const zchar_t *channel, zbool_t enable)
{ return serial_recvmsg_enable(cast(zt), channel, enable); }

static zcm_retcode_t _serial_recvmsg(zcm_trans_t *zt, zcm_msg_t *msg, zint32_t timeout)
{ return serial_recvmsg(cast(zt), msg, timeout); }

static zcm_retcode_t _serial_update(zcm_trans_t *zt)
{
    zcm_retcode_t rxRet = serial_update_rx(zt);
    zcm_retcode_t txRet = serial_update_tx(zt);
    return rxRet == ZCM_EOK ? txRet : rxRet;
}

static zcm_trans_methods_t methods = {
    &_serial_get_mtu,
    &_serial_sendmsg,
    &_serial_recvmsg_enable,
    &_serial_recvmsg,
    &_serial_update,
    &zcm_trans_generic_serial_destroy,
};

static zcm_trans_generic_serial_t *cast(zcm_trans_t *zt)
{
    assert(zt->vtbl == &methods);
    return (zcm_trans_generic_serial_t*)zt;
}

zcm_trans_t *zcm_trans_generic_serial_create(
        zuint32_t (*get)(zuint8_t* data, zuint32_t nData, void* usr),
        zuint32_t (*put)(const zuint8_t* data, zuint32_t nData, void* usr),
        void* put_get_usr,
        zuint64_t (*timestamp_now)(void* usr),
        void* time_usr,
        zuint32_t MTU,
        zuint32_t bufSize)
{
    if (MTU == 0 || bufSize < FRAME_BYTES + MTU) return NULL;
    zcm_trans_generic_serial_t *zt = zcm_malloc(sizeof(zcm_trans_generic_serial_t));
    if (zt == NULL) return NULL;
    zt->mtu = MTU;
    zt->recvMsgData = zcm_malloc(zt->mtu * sizeof(zuint8_t));
    if (zt->recvMsgData == NULL) {
        zcm_free(zt);
        return NULL;
    }

    zt->trans.trans_type = ZCM_NONBLOCKING;
    zt->trans.vtbl = &methods;
    if (!cb_init(&zt->sendBuffer, bufSize)) {
        zcm_free(zt->recvMsgData);
        zcm_free(zt);
        return NULL;
    }
    if (!cb_init(&zt->recvBuffer, bufSize)) {
        cb_deinit(&zt->sendBuffer);
        zcm_free(zt->recvMsgData);
        zcm_free(zt);
        return NULL;
    }

    zt->get = get;
    zt->put = put;
    zt->put_get_usr = put_get_usr;

    zt->time = timestamp_now;
    zt->time_usr = time_usr;

    return (zcm_trans_t*) zt;
}

void zcm_trans_generic_serial_destroy(zcm_trans_t* _zt)
{
    zcm_trans_generic_serial_t *zt = cast(_zt);
    cb_deinit(&zt->recvBuffer);
    cb_deinit(&zt->sendBuffer);
    zcm_free(zt->recvMsgData);
    zcm_free(zt);
}
