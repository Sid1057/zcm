#include "zcm/transport.h"
#include "zcm/transport_registrar.h"
#include "zcm/transport_register.hpp"

#include "zcm/util/debug.h"
#include "util/TimeUtil.hpp"

#include <algorithm>
#include <deque>
#include <mutex>
#include <condition_variable>

#define ZCM_TRANS_CLASSNAME TransportNonblockInproc
#define MTU (1<<28)

using namespace std;

struct ZCM_TRANS_CLASSNAME : public zcm_trans_t
{
    // Messages are queued into a deque and then dispatched one at a time through recvmsg
    // using the "inFlight" pointers to store their memory until the next message is dispatched
    // Note: Have to use free() to clean up chan memory in these because we create them via strdup
    deque<zcm_msg_t*> msgs;
    const zchar_t*  inFlightChanMem = nullptr;
          zuint8_t* inFlightDataMem = nullptr;

    condition_variable msgCond;
    mutex msgLock;

    ZCM_TRANS_CLASSNAME(zcm_url_t *url, zbool_t blocking)
    {
        trans_type = blocking ? ZCM_BLOCKING : ZCM_NONBLOCKING;
        vtbl = &methods;
    }

    ~ZCM_TRANS_CLASSNAME()
    {
        for (auto msg: msgs) {
            zcm_free((void*) msg->channel);
            delete [] msg->buf;
            delete msg;
        }
        msgs.clear();

        zcm_free((void*) inFlightChanMem);
        delete [] inFlightDataMem;
    }

    zbool_t good() { return ztrue; }

    /********************** METHODS **********************/
    zuint32_t get_mtu() { return MTU; }

    zcm_retcode_t sendmsg(zcm_msg_t msg)
    {
        zuint32_t chanLen = 0;
        for (; chanLen < ZCM_CHANNEL_MAXLEN + 1; ++chanLen) {
            if (msg.channel[chanLen] == '\0') break;
        }
        if (chanLen > ZCM_CHANNEL_MAXLEN) {
            ZCM_DEBUG("nonblock_inproc_send failed: invalid channel length");
            return ZCM_EINVALID;
        }
        if (msg.len > MTU) {
            ZCM_DEBUG("nonblock_inproc_send failed: msg larger than MTU");
            return ZCM_EINVALID;
        }

        zcm_msg_t *newMsg = new zcm_msg_t();
        newMsg->utime = msg.utime;
        newMsg->len = msg.len;
        newMsg->channel = strdup(msg.channel);
        newMsg->buf = new zuint8_t[msg.len];
        std::copy_n(msg.buf, msg.len, newMsg->buf);

        std::unique_lock<mutex> lk(msgLock, defer_lock);
        if (trans_type == ZCM_BLOCKING) lk.lock();
        msgs.push_back(newMsg);
        if (trans_type == ZCM_BLOCKING) {
            lk.unlock();
            msgCond.notify_all();
        }

        return ZCM_EOK;
    }

    zcm_retcode_t recvmsg_enable(const zchar_t *channel, zbool_t enable) { return ZCM_EOK; }

    zcm_retcode_t recvmsg(zcm_msg_t *msg, zint32_t timeout)
    {
        if (timeout < 0) timeout = std::numeric_limits<zint32_t>::max();

        std::unique_lock<mutex> lk(msgLock, defer_lock);

        if (trans_type == ZCM_BLOCKING) {
            lk.lock();
            zbool_t available = msgCond.wait_for(lk, chrono::milliseconds(timeout),
                                                 [&](){ return !msgs.empty(); });
            if (!available) return ZCM_EAGAIN;
        } else {
            if (msgs.empty()) return ZCM_EAGAIN;
        }

        // Clean up memory from last message
        zcm_free((void*) inFlightChanMem);
        delete [] inFlightDataMem;

        // Steal the dynamic memory from the front of the queue, but hang onto the
        // ptrs via the "inFlight" ptrs so we can clean it up later
        *msg = *(msgs.front());
        msg->utime = TimeUtil::utime();
        inFlightChanMem = msg->channel;
        inFlightDataMem = msg->buf;

        delete msgs.front();
        msgs.pop_front();

        return ZCM_EOK;
    }

    zcm_retcode_t update() { return ZCM_EOK; }

    /********************** STATICS **********************/
    static zcm_trans_methods_t methods;
    static ZCM_TRANS_CLASSNAME *cast(zcm_trans_t *zt)
    {
        assert(zt->vtbl == &methods);
        return (ZCM_TRANS_CLASSNAME*)zt;
    }

    static zuint32_t _get_mtu(zcm_trans_t *zt)
    { return cast(zt)->get_mtu(); }

    static zcm_retcode_t _sendmsg(zcm_trans_t *zt, zcm_msg_t msg)
    { return cast(zt)->sendmsg(msg); }

    static zcm_retcode_t _recvmsg_enable(zcm_trans_t *zt, const zchar_t *channel, zbool_t enable)
    { return cast(zt)->recvmsg_enable(channel, enable); }

    static zcm_retcode_t _recvmsg(zcm_trans_t *zt, zcm_msg_t *msg, zint32_t timeout)
    { return cast(zt)->recvmsg(msg, timeout); }

    static zcm_retcode_t _update(zcm_trans_t *zt)
    { return cast(zt)->update(); }

    static void _destroy(zcm_trans_t *zt)
    { delete cast(zt); }

    static const TransportRegister regBlocking;
    static const TransportRegister regNonblocking;
};

zcm_trans_methods_t ZCM_TRANS_CLASSNAME::methods = {
    &ZCM_TRANS_CLASSNAME::_get_mtu,
    &ZCM_TRANS_CLASSNAME::_sendmsg,
    &ZCM_TRANS_CLASSNAME::_recvmsg_enable,
    &ZCM_TRANS_CLASSNAME::_recvmsg,
    &ZCM_TRANS_CLASSNAME::_update,
    &ZCM_TRANS_CLASSNAME::_destroy,
};

static zcm_trans_t *create_blocking(zcm_url_t *url)
{ return new ZCM_TRANS_CLASSNAME(url, true); }

static zcm_trans_t *create_nonblocking(zcm_url_t *url)
{ return new ZCM_TRANS_CLASSNAME(url, false); }

const TransportRegister ZCM_TRANS_CLASSNAME::regBlocking(
    "block-inproc",
    "Blocking in-process deterministic transport",
    create_blocking);

const TransportRegister ZCM_TRANS_CLASSNAME::regNonblocking(
    "nonblock-inproc",
    "Nonblocking in-process deterministic transport. NOT INTERNALLY THREADSAFE",
    create_nonblocking);
